
#include <stdlib.h>
#include "driver/gpio.h"
#include "include/stepper.h"

static enum coil_state {
  step_one, step_two, step_three, step_four
} coil_state;

Stepper * Stepper_Init(int pin1, int pin2, int pin3, int pin4)
{
  Stepper *stepper = malloc(sizeof(Stepper));

  stepper->number_of_steps = 0;
  stepper->direction = 0;
  stepper->step_number = 0;
  stepper->last_step_time = 0;
  stepper->step_delay = 0;
  stepper->coil_state = step_one;

  stepper->coil_a_dir = pin1;
  stepper->coil_a_dir_inv = pin2;
  stepper->coil_b_dir = pin3;
  stepper->coil_b_dir_inv = pin4;

  // Configure GPIO
  uint32_t gpio_output_pin = (1ULL<<pin1) | (1ULL<<pin2) |(1ULL<<pin3) |(1ULL<<pin4);
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = gpio_output_pin;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  return stepper;
}


/* @Function: Stepper_SetSpeed
 * @brief Set speed in revolutions per minute
 **/
void Stepper_SetSpeed(Stepper * stepper, int speed)
{
  stepper->step_delay = 60L * 1000L * 1000L / stepper->number_of_steps / speed;
}

/* @Function: Stepper_StepMotor
 * @brief Move motor forward or backwards
 **/
void Stepper_StepMotor(Stepper * stepper)
{
  switch (stepper->coil_state) {
    case step_one:
      stepper->coil_state = step_two;
      break;
    case step_two:
      stepper->coil_state = step_three;
      break;
    case step_three:
      stepper->coil_state = step_four;
      break;
    case step_four:
      stepper->coil_state = step_one;
      break;
    default:
      break;
  }
}
