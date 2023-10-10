
/* █▀ ▀█▀ █▀▀ █▀█ █▀█ █▀▀ █▀█    █▀▄▀█ █▀█ ▀█▀ █▀█ █▀█    █▀▄▀█ █░█ █▀ █ █▀▀ */
/* ▄█ ░█░ ██▄ █▀▀ █▀▀ ██▄ █▀▄    █░▀░█ █▄█ ░█░ █▄█ █▀▄    █░▀░█ █▄█ ▄█ █ █▄▄ */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "soc/gpio_reg.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "midi.h"
#include "frequency.h"

/***** Specify Midi file and length here ******/
#include "midi/midi_god.h"
#define MIDI_LEN (god_mid_len)
#define MIDI_ARR (god_mid)
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
TaskHandle_t StepperTaskHandle;
esp_timer_handle_t pwm_timers[STEPPER_COUNT];
uint8_t steppers_active = 0;
gpio_num_t stepper_pins[STEPPER_COUNT]  = { STEPPER_0_PIN,   STEPPER_1_PIN,   STEPPER_2_PIN,   STEPPER_3_PIN };
uint8_t stepper_notes_playing[STEPPER_COUNT] = { STEPPER_NO_NOTE_PLAYING };
TickType_t stepper_last_played[STEPPER_COUNT] = { 0 };

// Queues
#define NOTE_QUEUE_SIZE STEPPER_COUNT
QueueHandle_t NoteQueue;

// Callback to generate PWM signal
static void timer_callback(void * arg)
{
  static uint8_t state = LOW;
  if (state == LOW) {
    REG_WRITE(GPIO_OUT_W1TS_REG, 1 << (*(gpio_num_t*) arg));
  } else {
    REG_WRITE(GPIO_OUT_W1TC_REG, 1 << (*(gpio_num_t*) arg));
  }
  state = !state;
}

void Stepper_NoteOff(uint8_t note)
{
  // Find a stepper that's playing this note and stop its PWM
  for (int i = 0; i < STEPPER_COUNT; i++) {
    if (stepper_notes_playing[i] == note) {
      // printf("Stopping Stepper%d\n", i);
      esp_timer_stop(pwm_timers[i]);
      gpio_set_level(stepper_pins[i], LOW);
      stepper_notes_playing[i] = STEPPER_NO_NOTE_PLAYING;
      steppers_active--;
      return;
    }
  }
}

// Responsible for adjusting timer (note) frequency
TaskHandle_t Stepper_TaskHandle = NULL;
void Stepper_Task(void * arg)
{
  midi_midi_event e;

  while (1) {
    // still unsure if this is the right function to call
    ulTaskNotifyTake(0, portMAX_DELAY);

    if (uxQueueMessagesWaiting(NoteQueue) > 0) {
      xQueueReceive(NoteQueue, &e, 5);
      if (e.status != MIDI_STATUS_NOTE_ON) continue;

      // Pick a stepper to send to
      uint8_t stepper_num = 0;
      if (steppers_active == STEPPER_COUNT) {
        Stepper_NoteOff(stepper_notes_playing[0]);
      }

      esp_timer_start_periodic(pwm_timers[stepper_num], period[e.param1] / 10);
      steppers_active++;
      stepper_notes_playing[stepper_num] = e.param1;
      stepper_last_played[stepper_num] = xTaskGetTickCount();
    }
  }
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

  // Midi controller variables
  uint8_t first_note_event = true;
  double mticks_per_ms = 0.25;
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
        mticks_per_ms = 1000.0 / parser->header.time_division;
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
          xQueueSend(NoteQueue, &stored_event, 5);
          xTaskNotifyGive(Stepper_TaskHandle);
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
    vTaskDelay((double) mticks_til_next_event * mticks_per_ms);
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

  // Queue for steppers to read notes from
  NoteQueue = xQueueCreate(NOTE_QUEUE_SIZE, sizeof(midi_midi_event * ));

  // Set up timers controlling PWM for each stepper
  for (int i = 0; i < STEPPER_COUNT; i++) {
    esp_timer_create_args_t periodic_timer_args = {
      .callback = &timer_callback,
      .arg = (void *) &stepper_pins[i],
    };

    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &pwm_timers[i]));
  }

  xTaskCreate(Stepper_Task, "StepperTask", 2048, NULL, 10, &Stepper_TaskHandle);
  xTaskCreate(MidiController_Task, "MidiController_Task", 4096, NULL, 10, &MidiController_TaskHandle);
}

