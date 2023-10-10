
/* █▀ ▀█▀ █▀▀ █▀█ █▀█ █▀▀ █▀█    █▀▄▀█ █▀█ ▀█▀ █▀█ █▀█    █▀▄▀█ █░█ █▀ █ █▀▀ */
/* ▄█ ░█░ ██▄ █▀▀ █▀▀ ██▄ █▀▄    █░▀░█ █▄█ ░█░ █▄█ █▀▄    █░▀░█ █▄█ ▄█ █ █▄▄ */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "soc/gpio_reg.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "midi.h"
#include "frequency.h"

/***** Specify Midi file and length here ******/
// #include "midi/god_multi.h"
// #define MIDI_LEN (playing_god_multi_mid_len)
// #define MIDI_ARR (playing_god_multi_mid)
#include "midi/midi_god.h"
#define MIDI_LEN (god_mid_len)
#define MIDI_ARR (god_mid)
/**********************************************/

#define LOW  0
#define HIGH 1
#define STEPPER_COUNT 4

#define TEMPO_BYTE_LENGTH 3
#define STEPPER_NO_NOTE_PLAYING 128 // valid midi note range is 0-127

// Pin definitions
#define DEBUG_LED GPIO_NUM_2 
#define STEPPER_0_PIN GPIO_NUM_23
#define STEPPER_1_PIN GPIO_NUM_1
#define STEPPER_2_PIN GPIO_NUM_21
#define STEPPER_3_PIN GPIO_NUM_18
#define GPIO_OUT_PINS ((1ULL << STEPPER_0_PIN) | (1ULL << STEPPER_1_PIN)| \
                       (1ULL << STEPPER_2_PIN) | (1ULL << STEPPER_3_PIN) | (1ULL << DEBUG_LED))

#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES   LEDC_TIMER_3_BIT // only 50% duty available

// Stepper array variables
TaskHandle_t StepperTaskHandle;
uint8_t steppers_active = 0;

gpio_num_t     pins[STEPPER_COUNT]     = { STEPPER_0_PIN,   STEPPER_1_PIN,  STEPPER_2_PIN,  STEPPER_3_PIN };
ledc_channel_t channels[STEPPER_COUNT] = { LEDC_CHANNEL_0,  LEDC_CHANNEL_1, LEDC_CHANNEL_2, LEDC_CHANNEL_3 };
ledc_timer_t   timers[STEPPER_COUNT]   = { LEDC_TIMER_0,    LEDC_TIMER_1,   LEDC_TIMER_2,   LEDC_TIMER_3 };

TickType_t stepper_last_played[STEPPER_COUNT] = { 0 };
uint8_t notes_playing[STEPPER_COUNT] = { STEPPER_NO_NOTE_PLAYING };

// this should probably not be a global var 
// but idk what freertos construct to use
static midi_midi_event current_event;

/* @function Stepper_NoteOff
 * @param note - Note number (0-127) to turn off
 * @brief Stops a note by finding a stepper playing that note and pausing its timer */
void Stepper_NoteOff(uint8_t note)
{
  for (int i = 0; i < STEPPER_COUNT; i++) {
    if (notes_playing[i] == note) {
      ESP_ERROR_CHECK(ledc_timer_pause(LEDC_MODE, timers[i]));
      notes_playing[i] = STEPPER_NO_NOTE_PLAYING;
      stepper_last_played[i] = 0;
      steppers_active--;
      return;
    }
  }
}

/* @function AssignStepper
 * @brief Pick a stepper that the next note should be assigned to
 * @return Index of chosen stepper */
uint8_t AssignStepper(void)
{
  if (steppers_active == STEPPER_COUNT) {
    // If all are active - pick the LRU stepper
    uint32_t min = stepper_last_played[0];
    uint8_t min_idx = 0;
    for (int i = 0; i < STEPPER_COUNT; i++) {
      if (stepper_last_played[i] != 0 && stepper_last_played[i] < min) {
        min = stepper_last_played[i];
        min_idx = i;
      }
    }
    Stepper_NoteOff(notes_playing[min_idx]);
    return min_idx;
  } else {
    for (int i = 0; i < STEPPER_COUNT; i++) {
      if (notes_playing[i] == STEPPER_NO_NOTE_PLAYING) return i;
    }
  }
  return 0;
}

/* @function Stepper_Task
 * @brief Given a note (for now: in global var midi_midi_event current_event),
 *        determine which stepper should play the note, and update the stepper 
 *        frequency accordingly. */
TaskHandle_t Stepper_TaskHandle = NULL;
void Stepper_Task(void * arg)
{
  while (1) {
    ulTaskNotifyTake(0, portMAX_DELAY);
    if (current_event.status != MIDI_STATUS_NOTE_ON) continue;
    uint8_t num = AssignStepper();
    ESP_ERROR_CHECK(ledc_set_freq(LEDC_MODE, timers[num], (int) frequency[current_event.param1]));
    ESP_ERROR_CHECK(ledc_timer_resume(LEDC_MODE, timers[num]));
    steppers_active++;
    notes_playing[num] = current_event.param1;
    stepper_last_played[num] = xTaskGetTickCount();
  }
}

/* @function MidiController_Task
 * @brief Run the midi parser. */
TaskHandle_t MidiController_TaskHandle = NULL;
void MidiController_Task(void * arg)
{
  // Initialize
  struct midi_parser * parser = malloc(sizeof(struct midi_parser));
  parser->state = MIDI_PARSER_INIT;
  parser->size  = MIDI_LEN;
  parser->in    = MIDI_ARR;
  enum midi_parser_status status;

  // Midi controller variables
  uint8_t first_note_event = true;
  uint16_t mticks_per_task_tick = 20; // default: 120 bpm 4/4
  uint16_t mticks_til_next_event = 0;
  midi_midi_event stored_event;
  uint32_t us_per_qtr = 0; // from set_tempo meta event

  while (1)
  {
    status = midi_parse(parser);
    switch (status) {
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
          current_event = stored_event;
          xTaskNotifyGive(Stepper_TaskHandle);
        } else if (stored_event.status == MIDI_STATUS_NOTE_OFF) {
          Stepper_NoteOff(stored_event.param1);
        }

        // Set delay time til next event
        mticks_til_next_event = parser->vtime;

        // Store current event
        stored_event = parser->midi;
        break;

      case MIDI_PARSER_TRACK_META:
        if (parser->meta.type == MIDI_META_SET_TEMPO) {
          // Read next 3 bytes for tempo (usec per qtr note)
          us_per_qtr = 0;
          for (int i = 0; i < TEMPO_BYTE_LENGTH; i++) {
            us_per_qtr <<= 8;
            us_per_qtr += parser->meta.bytes[i];
          }
          if (us_per_qtr == 0) break;
          mticks_per_task_tick = (parser->header.time_division * (1000000/configTICK_RATE_HZ)) / us_per_qtr;
        }
        break;

      default: break;
    }

    // Only run task when next event is supposed to happen
    int delay_ticks = mticks_til_next_event / mticks_per_task_tick;
    vTaskDelay(delay_ticks);
  }
}

void app_main()
{
  for (int i = 0; i < STEPPER_COUNT; i++) {
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
      .speed_mode       = LEDC_MODE,
      .timer_num        = timers[i],
      .duty_resolution  = LEDC_DUTY_RES,
      .freq_hz          = 1000,
      .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
      .speed_mode = LEDC_MODE,
      .channel    = channels[i],
      .timer_sel  = timers[i],
      .intr_type  = LEDC_INTR_DISABLE,
      .gpio_num   = pins[i],
      .duty       = 3,
      .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ESP_ERROR_CHECK(ledc_timer_pause(LEDC_MODE, timers[i]));
  }

  xTaskCreate(Stepper_Task, "StepperTask", 4096, NULL, 10, &Stepper_TaskHandle);
  xTaskCreate(MidiController_Task, "MidiController_Task", 4096, NULL, 10, &MidiController_TaskHandle);
}
