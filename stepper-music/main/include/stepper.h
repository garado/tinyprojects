
/* █▀ ▀█▀ █▀▀ █▀█ █▀█ █▀▀ █▀█ */
/* ▄█ ░█░ ██▄ █▀▀ █▀▀ ██▄ █▀▄ */

#ifndef __STEPPER_H__
#define __STEPPER_H__

#include <inttypes.h>
#include "midi.h"

#define STEPPER_COUNT 4
#define STEPPER_0_PIN GPIO_NUM_23
#define STEPPER_1_PIN GPIO_NUM_1
#define STEPPER_2_PIN GPIO_NUM_21
#define STEPPER_3_PIN GPIO_NUM_18
#define GPIO_OUT_PINS ((1ULL << STEPPER_0_PIN) | (1ULL << STEPPER_1_PIN)| \
                       (1ULL << STEPPER_2_PIN) | (1ULL << STEPPER_3_PIN) | (1ULL << DEBUG_LED))

#define LEDC_MODE       LEDC_LOW_SPEED_MODE

// These are the best settings to use.
// With this you get almost the full range: C1 through G9
#define LEDC_DUTY_RES   LEDC_TIMER_6_BIT
#define LEDC_DUTY       31

#define OCTAVE 12
#define LOWEST_PLAYABLE_NOTE  12

#define STEPPER_NO_NOTE_PLAYING 128 // valid midi note range is 0-127

void Stepper_Init(void);

uint8_t Stepper_Select(void);

void Stepper_AllOff(void);

void Stepper_NoteOff(uint8_t note);

void Stepper_NoteOn(midi_midi_event e);
  
void Stepper_FrequencyTest(void);

#endif
