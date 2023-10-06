
/* █▀ ▀█▀ █▀▀ █▀█ █▀█ █▀▀ █▀█    █▀ █▄█ █▀▄▀█ █▀█ █░█ █▀█ █▄░█ █▄█ */
/* ▄█ ░█░ ██▄ █▀▀ █▀▀ ██▄ █▀▄    ▄█ ░█░ █░▀░█ █▀▀ █▀█ █▄█ █░▀█ ░█░ */

#include <stdio.h>
#include <stdint.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "hal/ledc_types.h"
#include "esp_log.h"
#include "include/midi.h"

#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES   LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY  (440) // Hz
#define LEDC_DUTY       4095

#define LOW  0
#define HIGH 1  

// Frequency
#define A4      440
#define CSHARP5 554 // 554.37 
#define E5      659 // 659.25

// #define A1        55
// #define CSHARP1   32 // 32.70
// #define E2        82 // 82.41

#define A1        220
#define CSHARP1   277
#define E2        165

// Pin definitions
#define STEPPER_0_PIN GPIO_NUM_23
#define STEPPER_1_PIN GPIO_NUM_1
#define STEPPER_2_PIN GPIO_NUM_21
#define STEPPER_3_PIN GPIO_NUM_18
#define STEPPER_4_PIN GPIO_NUM_17
#define GPIO_OUT_PINS ((1ULL << STEPPER_0_PIN) | (1ULL << STEPPER_1_PIN)| (1ULL << STEPPER_2_PIN)| \
                       (1ULL << STEPPER_3_PIN)| (1ULL << STEPPER_4_PIN))

static void Stepper_Init(int pin, int chan, int timer)
{
  // PWM timer configuration
  ledc_timer_config_t ledc_timer = {
    .speed_mode       = LEDC_MODE,
    .timer_num        = timer,
    .duty_resolution  = LEDC_DUTY_RES,
    .freq_hz          = LEDC_FREQUENCY,
    .clk_cfg          = LEDC_AUTO_CLK
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = {
    .speed_mode     = LEDC_MODE,
    .channel        = chan,
    .timer_sel      = timer,
    .intr_type      = LEDC_INTR_DISABLE,
    .gpio_num       = pin,
    .duty           = LEDC_DUTY,
    .hpoint         = 0
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

TaskHandle_t Stepper0_TaskHandle = NULL;
void Stepper0_Task(void * arg)
{
  Stepper_Init(STEPPER_0_PIN, LEDC_CHANNEL_0, LEDC_TIMER_0);
  ledc_set_freq(LEDC_MODE, LEDC_TIMER_0, CSHARP1);
  while (1) { vTaskDelay(1); }
}

TaskHandle_t Stepper1_TaskHandle = NULL;
void Stepper1_Task(void * arg)
{
  Stepper_Init(STEPPER_1_PIN, LEDC_CHANNEL_1, LEDC_TIMER_1);
  ledc_set_freq(LEDC_MODE, LEDC_TIMER_1, A1);
  while (1) { vTaskDelay(1); }
}

TaskHandle_t Stepper2_TaskHandle = NULL;
void Stepper2_Task(void * arg)
{
  Stepper_Init(STEPPER_2_PIN, LEDC_CHANNEL_2, LEDC_TIMER_2);
  ledc_set_freq(LEDC_MODE, LEDC_TIMER_2, E2);
  while (1) { vTaskDelay(1); }
}

void app_main(void)
{
  // Configure trigger pin
  gpio_config_t io_conf = {};
  io_conf.pin_bit_mask = GPIO_OUT_PINS;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);
  
  xTaskCreate(Stepper0_Task, "Stepper0_Task", 4096, NULL, 10, &Stepper0_TaskHandle);
  xTaskCreate(Stepper1_Task, "Stepper1_Task", 4096, NULL, 10, &Stepper1_TaskHandle);
  xTaskCreate(Stepper2_Task, "Stepper2_Task", 4096, NULL, 10, &Stepper2_TaskHandle);
}
