
/* █▀ ▀█▀ █▀▀ █▀█ █▀█ █▀▀ █▀█ */
/* ▄█ ░█░ ██▄ █▀▀ █▀▀ ██▄ █▀▄ */

#include <inttypes.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "stepper.h"
#include "frequency.h"

#define LEDC_ENABLE

// Stepper variables
gpio_num_t     pins[STEPPER_COUNT]     = { STEPPER_0_PIN,   STEPPER_1_PIN,  STEPPER_2_PIN,  STEPPER_3_PIN };
ledc_channel_t channels[STEPPER_COUNT] = { LEDC_CHANNEL_0,  LEDC_CHANNEL_1, LEDC_CHANNEL_2, LEDC_CHANNEL_3 };
ledc_timer_t   timers[STEPPER_COUNT]   = { LEDC_TIMER_0,    LEDC_TIMER_1,   LEDC_TIMER_2,   LEDC_TIMER_3 };
uint8_t steppers_active = 0;
uint8_t notes_playing[STEPPER_COUNT] = { STEPPER_NO_NOTE_PLAYING };
uint32_t time_last_played[STEPPER_COUNT] = { 0 };

void Stepper_Init()
{
  for (int i = 0; i < STEPPER_COUNT; i++) {
    notes_playing[i] = STEPPER_NO_NOTE_PLAYING;
  }

  #ifdef LEDC_ENABLE
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
      .duty       = LEDC_DUTY,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ESP_ERROR_CHECK(ledc_timer_pause(LEDC_MODE, timers[i]));
  }
  #endif
}

/* @function Stepper_Select
 * @brief Pick a stepper that the next note should be assigned to
 * @return Index of chosen stepper */
uint8_t Stepper_Select(void)
{
  if (steppers_active == STEPPER_COUNT) {
    // If all are active - pick the LRU stepper
    uint32_t min = time_last_played[0];
    uint8_t min_idx = 0;
    for (int i = 0; i < STEPPER_COUNT; i++) {
      if (time_last_played[i] != 0 && time_last_played[i] < min) {
        min = time_last_played[i];
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
  printf("ERROR ASSIGN STEPPER\n");
  return 0;
}


/* @function Stepper_NoteOff
 * @param note - Note number (0-127) to turn off
 * @brief Stops a note by finding a stepper playing that note and pausing its timer */
void Stepper_NoteOff(uint8_t note)
{
  for (int i = 0; i < STEPPER_COUNT; i++) {
    if (notes_playing[i] == note) {
      #ifdef LEDC_ENABLE
      ESP_ERROR_CHECK(ledc_timer_pause(LEDC_MODE, timers[i]));
      #endif
      notes_playing[i] = STEPPER_NO_NOTE_PLAYING;
      time_last_played[i] = 0;
      steppers_active--;
      return;
    }
  }
}

/* @function Stepper_NoteOn
 * @brief Given a note (for now: in global var midi_midi_event e),
 *        determine which stepper should play the note, and update the stepper 
 *        frequency accordingly. */
void Stepper_NoteOn(midi_midi_event e)
{
  if (steppers_active == STEPPER_COUNT) return;
  uint8_t num = Stepper_Select();
  if (e.param1 < LOWEST_PLAYABLE_NOTE) e.param1 += OCTAVE;
  #ifdef LEDC_ENABLE
  ESP_ERROR_CHECK(ledc_set_freq(LEDC_MODE, timers[num], (int) frequency[e.param1]));
  ESP_ERROR_CHECK(ledc_timer_resume(LEDC_MODE, timers[num]));
  #endif
  steppers_active++;
  // printf("Set %d to %d (Active: %d)\n", num, e.param1, steppers_active);
  notes_playing[num] = e.param1;
  time_last_played[num] = xTaskGetTickCount();
}

void Stepper_AllOff(void)
{
  for (int i = 0; i < STEPPER_COUNT; i++) {
    if (notes_playing[i] != STEPPER_NO_NOTE_PLAYING) {
      Stepper_NoteOff(notes_playing[i]);
    }
  }
}

/* @function Stepper_FrequencyTest(void)
 * @brief Runs through all possible notes from highest to lowest on the first stepper. */
void Stepper_FrequencyTest(void)
{
  ESP_ERROR_CHECK(ledc_timer_resume(LEDC_MODE, timers[0]));
  for (int i = 127; i >= 0; i--) {
    printf("Note %d: %s%d (%dHz)\n", i, note_names[i % 12], i / 12, (int) frequency[i]);
    ESP_ERROR_CHECK(ledc_set_freq(LEDC_MODE, timers[0], (int) frequency[i]));
    vTaskDelay(250 / portTICK_PERIOD_MS);
  }
}
