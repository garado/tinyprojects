
#include <stdlib.h>
#include "driver/gpio.h"
#include "include/stepper.h"

#define HIGH 1
#define LOW 0

static enum coil_state {
  step_one, step_two, step_three, step_four
} coil_state;

static enum {
  off, inited, stepping, halted,
} stepper_state = off;

Stepper * Stepper_Init(int pin1, int pin2, int pin3, int pin4)
{
  Stepper *s = malloc(sizeof(Stepper));

  s->stepper_state = off;
  s->coil_state = step_one;
  s->steps_per_second_rate = DEFAULT_STEP_RATE;
  s->step_count = 0;
  s->overflow_reps = 0;

  s->coil_a_dir = pin1;
  s->coil_a_dir_inv = pin2;
  s->coil_b_dir = pin3;
  s->coil_b_dir_inv = pin4;

  // Configure GPIO
  uint32_t gpio_output_pin = (1ULL<<pin1) | (1ULL<<pin2) |(1ULL<<pin3) |(1ULL<<pin4);
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = gpio_output_pin;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  gpio_set_level(s->coil_a_dir, HIGH);
  gpio_set_level(s->coil_a_dir_inv, LOW);
  gpio_set_level(s->coil_b_dir, HIGH);
  gpio_set_level(s->coil_b_dir_inv, LOW);

  s->stepper_state = inited;

  return s;
}


/* @Function: Stepper_SetSpeed
 * @brief Set speed in revolutions per minute
 **/
void Stepper_SetSpeed(Stepper * stepper, int speed)
{
//   stepper->step_delay = 60L * 1000L * 1000L / stepper->number_of_steps / speed;
}

/* @Function: Stepper_SetRate
 * @brief Sets the stepping rate in steps per second.
 **/
void Stepper_SetRate(Stepper * s, uint16_t rate)
{
  uint16_t overflowPeriod;

  // uint16_t overflowPeriod;
  // stepsPerSecondRate = rate;
  // if ((rate > TWENTY_KILOHERTZ)) {
  //   return ERROR;
  // }
  // T3CONbits.ON = 0; // halt timer3
  // overflowPeriod = CalculateOverflowPeriod(rate);
  // PR3 = overflowPeriod;
  // if (stepperState != halted) {
  //   T3CONbits.ON = 1; // restart timer3
  // }
  // return SUCCESS;
}

/* @Function: Stepper_StepMotor
 * @brief Move motor forward or backwards
 **/
void Stepper_StepMotor(Stepper * s)
{
  switch (s->coil_state) {
    case step_one: // 1010
      gpio_set_level(s->coil_a_dir,     HIGH);
      gpio_set_level(s->coil_a_dir_inv, LOW);
      gpio_set_level(s->coil_b_dir,     HIGH);
      gpio_set_level(s->coil_b_dir_inv, LOW);
      s->coil_state = step_two;
      break;
    case step_two: // 1001
      gpio_set_level(s->coil_a_dir,     LOW);
      gpio_set_level(s->coil_a_dir_inv, HIGH);
      gpio_set_level(s->coil_b_dir,     HIGH);
      gpio_set_level(s->coil_b_dir_inv, LOW);
      s->coil_state = step_three;
      break;
    case step_three: // 0101
      gpio_set_level(s->coil_a_dir,     LOW);
      gpio_set_level(s->coil_a_dir_inv, HIGH);
      gpio_set_level(s->coil_b_dir,     LOW);
      gpio_set_level(s->coil_b_dir_inv, HIGH);
      s->coil_state = step_four;
      break;
    case step_four: // 0110
      gpio_set_level(s->coil_a_dir,     HIGH);
      gpio_set_level(s->coil_a_dir_inv, LOW);
      gpio_set_level(s->coil_b_dir,     LOW);
      gpio_set_level(s->coil_b_dir_inv, HIGH);
      s->coil_state = step_one;
      break;
    default: break;
  }
}
