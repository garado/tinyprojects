
/* █▀ ▀█▀ █▀▀ █▀█ █▀█ █▀▀ █▀█    █▀▄▀█ █▀█ ▀█▀ █▀█ █▀█    █▀▄▀█ █░█ █▀ █ █▀▀ */
/* ▄█ ░█░ ██▄ █▀▀ █▀▀ ██▄ █▀▄    █░▀░█ █▄█ ░█░ █▄█ █▀▄    █░▀░█ █▄█ ▄█ █ █▄▄ */

/***** Keep the following lines on line 6-8 ******/
#include "midi/corridors_of_time.h"
#define MIDI_LEN (corridors_of_time_mid_len)
#define MIDI_ARR (corridors_of_time_mid)
/**********************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "soc/gpio_reg.h"
#include "esp_log.h"

#include "midi.h"
#include "stepper.h"

#define LOW  0
#define HIGH 1
#define TEMPO_BYTE_LENGTH 3
#define DEBUG_LED GPIO_NUM_2

TaskHandle_t MidiController_TaskHandle = NULL;
gptimer_handle_t parser_timer = NULL;

/* @function MidiController_Task
 * @brief Run the midi parser. */
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
  uint16_t mticks_til_next_event = 0;
  midi_midi_event stored_event;
  uint32_t us_per_qtr = 0; // from set_tempo meta event

  uint16_t mticks_per_task_tick = 20; // default: 120 bpm 4/4

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
        if (stored_event.status == MIDI_STATUS_NOTE_ON) {
          Stepper_NoteOn(stored_event);
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

      case MIDI_PARSER_EOB:
        Stepper_AllOff();
        vTaskDelete(MidiController_TaskHandle);
        break;

      default: break;
    }

    uint16_t delay_ticks = mticks_til_next_event / mticks_per_task_tick;
    vTaskDelay(delay_ticks);
  }
}

void app_main()
{
  Stepper_Init();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  xTaskCreate(MidiController_Task, "MidiController_Task", 4096, NULL, 10, &MidiController_TaskHandle);
}

