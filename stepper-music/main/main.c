
/* █▀ ▀█▀ █▀▀ █▀█ █▀█ █▀▀ █▀█    █▀▄▀█ █░█ █▀ █ █▀▀ */
/* ▄█ ░█░ ██▄ █▀▀ █▀▀ ██▄ █▀▄    █░▀░█ █▄█ ▄█ █ █▄▄ */

#include <stdio.h>
#include <stdint.h>
#include "include/stepper.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define STEPPER1_A GPIO_NUM_23
#define STEPPER1_B GPIO_NUM_22
#define STEPPER1_C GPIO_NUM_1
#define STEPPER1_D GPIO_NUM_3

void app_main(void)
{
  Stepper * stepper_1 = Stepper_Init(STEPPER1_A, STEPPER1_B, STEPPER1_C, STEPPER1_D);

  stepper_1->step_count = 440;

  while (1) {
    esp_rom_delay_us(2272);
    Stepper_StepMotor(stepper_1);
  }
}
