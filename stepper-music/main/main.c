
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

/***** Specify Midi file and length here ******/
#include "midi/god_multi.h"
#define MIDI_LEN (playing_god_multi_mid_len)
#define MIDI_ARR (playing_god_multi_mid)
/**********************************************/

#define LOW  0
#define HIGH 1
#define STEPPER_COUNT 4
#define STEPPER_NO_NOTE_PLAYING 128 // valid midi note range is 0-127

// Pin definitions
#define DEBUG_LED GPIO_NUM_2 
#define STEPPER_0_PIN GPIO_NUM_23
#define STEPPER_1_PIN GPIO_NUM_1
#define STEPPER_2_PIN GPIO_NUM_21
#define STEPPER_3_PIN GPIO_NUM_18
#define GPIO_OUT_PINS ((1ULL << STEPPER_0_PIN) | (1ULL << STEPPER_1_PIN)| \
                       (1ULL << STEPPER_2_PIN) | (1ULL << STEPPER_3_PIN) | (1ULL << DEBUG_LED))

static const char * TAG = "Step";

// Stepper array variables
static const char* stepper_task_names[] = { "Stepper0_Task", "Stepper1_Task", "Stepper2_Task", "Stepper3_Task", };
TaskHandle_t stepper_task_handles[] = { NULL };
uint8_t stepper_numbers[] = { 0, 1, 2, 3 };
gpio_num_t stepper_pins[STEPPER_COUNT]  = { STEPPER_0_PIN,   STEPPER_1_PIN,   STEPPER_2_PIN,   STEPPER_3_PIN };
uint8_t stepper_notes_playing[STEPPER_COUNT] = { STEPPER_NO_NOTE_PLAYING };
gptimer_handle_t pwm_timers[STEPPER_COUNT];
TickType_t stepper_last_played[STEPPER_COUNT] = { 0 };

// Queues
#define NOTE_QUEUE_SIZE STEPPER_COUNT
QueueHandle_t NoteQueue;

// Callback to generate PWM signal
static bool IRAM_ATTR timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
  static uint8_t state = LOW;
  gpio_set_level(*((gpio_num_t*) user_data), state);
  state = !state;
  return pdFALSE;
}

// Responsible for adjusting timer (note) frequency
TaskHandle_t Stepper_TaskHandle = NULL;
void Stepper_Task(void * arg)
{
  midi_midi_event e;
  uint8_t stepper_num = *((uint8_t*) arg);

  while (1) {
    vTaskDelay(5);
    if (uxQueueMessagesWaiting(NoteQueue) > 0) {
      xQueueReceive(NoteQueue, &e, 5);
      if (e.status != MIDI_STATUS_NOTE_ON) continue;

      // Update frequency
      gptimer_alarm_config_t cfg = {
        .reload_count = 0,
        .alarm_count = period[e.param1 + 22], // FIXME
        .flags.auto_reload_on_alarm = true,
      };
      ESP_ERROR_CHECK(gptimer_set_alarm_action(pwm_timers[stepper_num], &cfg));
      ESP_ERROR_CHECK(gptimer_start(pwm_timers[stepper_num]));

      stepper_notes_playing[stepper_num] = e.param1;
      stepper_last_played[stepper_num] = xTaskGetTickCount();

      printf("STEPPER%d: N_On %d\n", stepper_num, e.param1);
    }
  }
}

void Stepper_NoteOff(uint8_t note)
{
  // Find a stepper that's playing this note and stop its PWM
  for (int i = 0; i < STEPPER_COUNT; i++) {
    if (stepper_notes_playing[i] == note) {
      printf("Stopping Stepper%d\n", i);
      gptimer_stop(pwm_timers[i]);
      gpio_set_level(stepper_pins[i], LOW);
      stepper_notes_playing[i] = STEPPER_NO_NOTE_PLAYING;
      return;
    }
  }
  ESP_LOGE(TAG, "There is no stepper playing note #%d", note);
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

  // Blargh
  uint8_t first_note_event = true;
  int mticks_per_ms = 1;
  int mticks_til_next_event = 0;
  midi_midi_event stored_event;

  while (1)
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
        // Determine ms per mtick. (tempo hardcoded to 120bpm for now)
        // 1000000 == 120bpm (tempo), then divide by 10^3 to get ms
        // mticks_per_ms = 1000 / parser->header.time_division;
        printf("mticks per ms: %d\n", mticks_per_ms);
        break;

      case MIDI_PARSER_TRACK_META:
        // TODO
        if (parser->meta.type == MIDI_META_SET_TEMPO) {
          // Update mticks per thingy
          // mticks_per_ms = (parser->meta.bytes / 1000) / parser->header.time_division;
          // printf("new mticks per ms: %d\n", mticks_per_ms);
        }
        break;

      case MIDI_PARSER_TRACK_MIDI:
        // Initialize the stored event if needed
        if (first_note_event) {
          stored_event = parser->midi;
          first_note_event = false;
          break;
        }

        // Handle the stored event
        // Only NOTE_ON handled for now.
        if (stored_event.status == MIDI_STATUS_NOTE_ON) {

          if (uxQueueMessagesWaiting(NoteQueue) == NOTE_QUEUE_SIZE) {
            // Find least recently played stepper
            for (int i = 0; i < STEPPER_COUNT; i++) {

            }
          }
          xQueueSend(NoteQueue, &stored_event, 5);
        } else if (stored_event.status == MIDI_STATUS_NOTE_OFF) {
          printf("Note off: %d\n", stored_event.param1);
          Stepper_NoteOff(stored_event.param1);
        }

        // Set delay time til next event
        mticks_til_next_event = parser->vtime;

        // Store current event
        stored_event = parser->midi;

        break;

      default: break;
    }

    // Run when next event happens
    vTaskDelay(mticks_til_next_event * mticks_per_ms);
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

  // Messages posted by MidiController.
  NoteQueue = xQueueCreate(NOTE_QUEUE_SIZE, sizeof(midi_midi_event * ));
  if (NoteQueue == NULL) {
    ESP_LOGE(TAG, "Failed to create NoteQueue");
  }

  for (int i = 0; i < 2; i++) {
    // Set up hardware timers controlling PWM for each stepper
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &pwm_timers[i]));
    gptimer_event_callbacks_t cbs = { .on_alarm = timer_callback };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(pwm_timers[i], &cbs, (void*) &stepper_pins[i]));
    ESP_ERROR_CHECK(gptimer_enable(pwm_timers[i]));

    // Create a task for each stepper
    xTaskCreate(Stepper_Task, stepper_task_names[i], 8092, (void *) &stepper_numbers[i], 10, &stepper_task_handles[i]);

    ESP_LOGI(TAG, "Finished Stepper %d setup\n", i);
  }

  xTaskCreate(MidiController_Task, "MidiController_Task", 8092, NULL, 10, &MidiController_TaskHandle);
}

