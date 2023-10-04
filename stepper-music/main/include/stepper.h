
/* █▀ ▀█▀ █▀▀ █▀█ █▀█ █▀▀ █▀█ */
/* ▄█ ░█░ ██▄ █▀▀ █▀▀ ██▄ █▀▄ */

// meant to be used with uln2003 stepper driver board
// use 2-wire control
// http://www.tigoe.com/pcomp/code/circuits/motors/stepper-motors/

#ifndef __STEPPER_H__
#define __STEPPER_H__

#include <stdint.h>

typedef struct Stepper {
  int number_of_steps;
  int direction;
  int step_number;
  unsigned long last_step_time;
  unsigned long step_delay;

  uint8_t coil_state;
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

/* @Function: Stepper_StepMotor
 * @brief Move motor forward or backwards
 **/
void Stepper_StepMotor(Stepper * stepper);

#endif
