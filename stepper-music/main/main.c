
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "soc/gpio_reg.h"

#include "midi.h"
#include "frequency.h"

// TODO: instead of 5v from wavegen just use 3.3v output from an esp32 pin

/***** Specify Midi file and length here ******/
#include "midi/ctest.h"
#define MIDI_LEN (ctest_mid_len)
#define MIDI_ARR (ctest_mid)
/**********************************************/

#define LOW  0
#define HIGH 1
#define STEPPER_COUNT 4

// Pin definitions
#define DEBUG_LED GPIO_NUM_2 
#define STEPPER_0_PIN GPIO_NUM_23
#define STEPPER_1_PIN GPIO_NUM_22 // TODO recehck pinout 
#define STEPPER_2_PIN GPIO_NUM_21
#define STEPPER_3_PIN GPIO_NUM_18
#define GPIO_OUT_PINS ((1ULL << STEPPER_0_PIN) | (1ULL << STEPPER_1_PIN)| \
                       (1ULL << STEPPER_2_PIN) | (1ULL << STEPPER_3_PIN) | (1ULL << DEBUG_LED))

static const char * TAG = "Step";

// Stepper array variables
gpio_num_t stepper_pins[STEPPER_COUNT] = { STEPPER_0_PIN, STEPPER_1_PIN, STEPPER_2_PIN, STEPPER_3_PIN };
gptimer_handle_t stepper_timers[STEPPER_COUNT] = { NULL }; // TODO: Rename to pwm_timers
gptimer_handle_t stepper_notes_playing[STEPPER_COUNT] = { 0 };

// Generate PWM signal
static bool IRAM_ATTR timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
  static uint8_t state = LOW;
  gpio_set_level(*((gpio_num_t*) user_data), state);
  state = !state;
  return pdFALSE;
}

#define PENDING_QUEUE_SIZE STEPPER_COUNT
#define NOTE_QUEUE_SIZE    STEPPER_COUNT
TimerHandle_t note_timers[NOTE_QUEUE_SIZE];
QueueHandle_t NoteQueue;
QueueHandle_t PendingNoteQueue;

// Responsible for starting/stopping timer and adjusting timer frequency
TaskHandle_t Stepper_TaskHandle = NULL;
void Stepper_Task(void * arg)
{
  midi_midi_event e;
  while (1) {
    vTaskDelay(10);
    if (uxQueueMessagesWaiting(NoteQueue) > 0) {
      xQueueReceive(NoteQueue, &e, 5);
      if (e.status == MIDI_STATUS_NOTE_OFF) {
        // ESP_ERROR_CHECK(gptimer_stop(stepper_timers[0]));
        // ESP_ERROR_CHECK(gptimer_set_alarm_action(stepper_timers[0], NULL));
        // gpio_set_level(STEPPER_0_PIN, LOW);
        // gptimer_disable(stepper_timers[0]);
        // not quite working yet!
        printf("N_Off: %d\n", e.param1);
      } else if (e.status == MIDI_STATUS_NOTE_ON){
        // gptimer_enable(stepper_timers[0]);
        ESP_ERROR_CHECK(gptimer_stop(stepper_timers[0]));
        gptimer_alarm_config_t cfg = {
          .reload_count = 0,
          .alarm_count = period[e.param1 + 22] / 2,
          .flags.auto_reload_on_alarm = true,
        };
        ESP_ERROR_CHECK(gptimer_set_alarm_action(stepper_timers[0], &cfg));
        ESP_ERROR_CHECK(gptimer_start(stepper_timers[0]));
        printf("N_On: %d (%d us)\n", e.param1, period[e.param1 + 22]);
      }
    }
  }
}

// Callback when midi timers expire.
void MidiTimerCallback( TimerHandle_t xTimer )
{
  midi_midi_event e;
  xQueueReceive(PendingNoteQueue, &e, 10);
  xQueueSend(NoteQueue, (void*) &e, (TickType_t) 10);
  // if (xQueueSend(NoteQueue, (void*) &e, (TickType_t) 10) == pdPASS) {
  //   printf("MTC: Send success\n");
  // }
}

TaskHandle_t MidiController_TaskHandle = NULL;
void MidiController_Task(void * arg)
{
  // Initialize Midi parser
  struct midi_parser * parser = malloc(sizeof(struct midi_parser));
  parser->state = MIDI_PARSER_INIT;
  parser->size  = MIDI_LEN;
  parser->in    = MIDI_ARR;
  enum midi_parser_status status;
 
  // Set up queues for Midi Controller and timers to post to.
 
  // Messages posted by MidiController.
  PendingNoteQueue = xQueueCreate(PENDING_QUEUE_SIZE, sizeof(midi_midi_event *));
  if (PendingNoteQueue == NULL) {
    ESP_LOGE(TAG, "Failed to create PendingNoteQueue");
  }

  // Messages posted by MidiTimer.
  NoteQueue = xQueueCreate(NOTE_QUEUE_SIZE, sizeof(midi_midi_event * ));
  if (NoteQueue == NULL) {
    ESP_LOGE(TAG, "Failed to create NoteQueue");
  }

  // Initialize timers that post to NoteQueue
  for (uint8_t i = 0; i < NOTE_QUEUE_SIZE; i++) {
    note_timers[i] = xTimerCreate("Timer0", 10, pdFALSE, (void*) 0, MidiTimerCallback);
  }

  uint64_t mticks_per_tick = 0;

  while (1)
  {
    vTaskDelay(5);

    if (uxQueueMessagesWaiting(PendingNoteQueue) < PENDING_QUEUE_SIZE)
    {
      status = midi_parse(parser);
      switch (status) {
        case MIDI_PARSER_EOB:
          ESP_LOGI(TAG, "Midi parsed");
          break;

        case MIDI_PARSER_ERROR:
          ESP_LOGE(TAG, "Midi parser error");
          return;

        case MIDI_PARSER_HEADER:
          mticks_per_tick = (parser->header.time_division * 4 * 100) / (600);
          printf("mticks per tick: %lld\n", mticks_per_tick);
          break;

        case MIDI_PARSER_TRACK_MIDI:
          // Post to PendingNoteQueue.
          // Once timer expires, it'll move the first message from pending note queue to NoteQueue 
          // where it will be received by the stepper tasks.
          // if (parser->midi.status != MIDI_STATUS_NOTE_ON && parser->midi.status != MIDI_STATUS_NOTE_OFF) continue;
          if (parser->midi.status != MIDI_STATUS_NOTE_ON) continue;

          if (xQueueSend(PendingNoteQueue, (void*) &(parser->midi), (TickType_t) 10) == pdPASS) {
            ESP_LOGI(TAG, "Pending %d in %lld mticks\n", parser->midi.param1, parser->vtime);

            // Set up timer that will handle event after `dt` timer has passed
            // todo: convert vtime to freertos ticks
            xTimerChangePeriod(note_timers[0], (parser->vtime / mticks_per_tick) + mticks_per_tick, 10);
            xTimerStart(note_timers[0], 10);
          }
          break;

        default: break;
      }
    }
  }
}

void app_main()
{
  // GPIO init
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = GPIO_OUT_PINS;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);
    
  // Create timer handle
  gptimer_config_t timer_config = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz = 1000000, // 1MHz, 1 tick=1us
  };

  // Init hardware timers controlling PWM for each stepper
  for (int i = 0; i < STEPPER_COUNT; i++) {
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &stepper_timers[i]));

    // Set up timer callbacks
    gptimer_event_callbacks_t cbs = { .on_alarm = timer_callback };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(stepper_timers[i], &cbs, (void*) &stepper_pins[i]));

    ESP_ERROR_CHECK(gptimer_enable(stepper_timers[i]));

    /// uuuuuuggghhh
    ESP_ERROR_CHECK(gptimer_start(stepper_timers[i]));
  }
 
  // Set up tasks
  xTaskCreate(MidiController_Task, "MidiController_Task", 4096, NULL, 10, &MidiController_TaskHandle);
  xTaskCreate(Stepper_Task, "Stepper_Task", 4096, NULL, 10, &Stepper_TaskHandle);
}
