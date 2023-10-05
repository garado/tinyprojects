
/* █▀ ▀█▀ █▀▀ █▀█ █▀█ █▀▀ █▀█ */
/* ▄█ ░█░ ██▄ █▀▀ █▀▀ ██▄ █▀▄ */

// meant to be used with uln2003 stepper driver board
// use 2-wire control
// http://www.tigoe.com/pcomp/code/circuits/motors/stepper-motors/

#include <stdint.h>

#ifndef __STEPPER_H__
#define __STEPPER_H__

#define ONE_HUNDRED_HZ 100
#define TWENTY_KILOHERTZ 20000
#define DEFAULT_STEP_RATE ONE_HUNDRED_HZ

typedef struct Stepper {
  int32_t step_count;
  uint32_t overflow_reps;
  uint8_t stepper_state;
  uint8_t coil_state;
  uint16_t steps_per_second_rate;

  // Pins
  int coil_a_dir;
  int coil_a_dir_inv;
  int coil_b_dir;
  int coil_b_dir_inv;

} Stepper;

/**
 * @Function: Stepper()
 * @param
 * @return ptr to new stepper struct
 * @brief Constructor for 4-pin stepper */
Stepper * Stepper_Init(int pin1, int pin2, int pin3, int pin4);

/* @Function: Stepper_SetSpeed
 * @brief Set speed in revolutions per minute
 **/
void Stepper_SetSpeed(Stepper * stepper, int speed);

/* @Function: Stepper_SetRate
 * @brief Sets the stepping rate in steps per second.
 **/
void Stepper_SetRate(Stepper * s, uint16_t rate);

/* @Function: Stepper_InitSteps
 * @brief Sets the steps and starts the stepper
 **/
void Stepper_InitSteps(Stepper * s, int32_t steps);

/* @Function: Stepper_StepMotor
 * @brief Move motor forward or backwards
 **/
void Stepper_StepMotor(Stepper * stepper);

#endif
