
/* █▀█ █▀█ █▄▄ █▀█ ▀█▀    ▄▀█ █▀█ █▀▄▀█ */
/* █▀▄ █▄█ █▄█ █▄█ ░█░    █▀█ █▀▄ █░▀░█ */

#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_continuous.h"
#include "peripherals.h"

typedef enum direction {
  NEUTRAL,
  INCREASE,
  DECREASE,
} direction;

static uint8_t base_adjust, claw_adjust, vertical_adjust, forward_adjust;

uint8_t * adjust_vars[SERVO_COUNT] = {
  &base_adjust, 
  &claw_adjust,
  &vertical_adjust,
  &forward_adjust,
};

TaskHandle_t adcTaskHandle = NULL;
TaskHandle_t servoTaskHandle = NULL;

/* @function s_conv_done_cb 
 * @brief Callback when adc is done converting */
static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
  BaseType_t mustYield = pdFALSE;
  vTaskNotifyGiveFromISR(adcTaskHandle, &mustYield);
  return (mustYield == pdTRUE);
}

void ADC_Task(void *arg)
{
  ADC_Init();

  esp_err_t ret;
  uint32_t ret_num = 0;
  uint8_t result[READ_LEN] = {0};
  memset(result, 0xcc, READ_LEN);
  
  adcTaskHandle = xTaskGetCurrentTaskHandle();

  adc_continuous_evt_cbs_t cbs = {
    .on_conv_done = s_conv_done_cb,
  };
  ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc_handle, &cbs, NULL));
  ESP_ERROR_CHECK(adc_continuous_start(adc_handle));

  while(1) {
    // Wait for adc callback before doing anything
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    ret = adc_continuous_read(adc_handle, result, READ_LEN, &ret_num, 0);
    if (ret == ESP_OK) {
      for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
        adc_digi_output_data_t *p = (adc_digi_output_data_t*)&result[i];
        uint32_t chan_num = ADC_GET_CHANNEL(p);
        uint32_t data = ADC_GET_DATA(p);

        uint8_t * adjust_var = NULL;

        if (chan_num == JOYSTICK_BASE_CHAN) {
          adjust_var = &base_adjust;
        } else if (chan_num == JOYSTICK_CLAW_CHAN) {
          adjust_var = &claw_adjust;
        } else if (chan_num == JOYSTICK_VERTICAL_CHAN) {
          adjust_var = &vertical_adjust;
        } else if (chan_num == JOYSTICK_FORWARD_CHAN) {
          adjust_var = &forward_adjust;
        }

        if (adjust_var == NULL) continue;

        uint8_t value;
        if (data < JOYSTICK_NEUTRAL_THRESHOLD_MIN) {
          // some of the servos are backwards/upside down :|
          value = chan_num == JOYSTICK_BASE_CHAN || chan_num == JOYSTICK_VERTICAL_CHAN ? INCREASE : DECREASE;
          *adjust_var = value;
        } else if (data > JOYSTICK_NEUTRAL_THRESHOLD_MAX) {
          value = chan_num == JOYSTICK_BASE_CHAN || chan_num == JOYSTICK_VERTICAL_CHAN ? DECREASE : INCREASE;
          *adjust_var = value;
        } else {
          *adjust_var = NEUTRAL;
        }
      }
    }
  }
}

void Servo_Task(void *arg)
{
  Servo_Init();

  //                                 Base   Claw   Vert  Forw
  uint16_t duty_min[SERVO_COUNT] = {  350,   660,  280,   200 };
  uint16_t duty_max[SERVO_COUNT] = { 1024,  1024,  720,   490 };
  uint8_t steps[SERVO_COUNT]     = {    8,    10,    8,     4 };

  // Initialize position
  uint16_t duty[SERVO_COUNT];
  for (int i = 0; i < SERVO_COUNT; i++) {
    duty[i] = (duty_max[i] - duty_min[i]) / 2;
    Servo_SetDutyCycle(i, duty[i]);
  };

  while(1) {
    vTaskDelay(pdMS_TO_TICKS(25));
    for (int i = 0; i < SERVO_COUNT; i++) {
      if (*adjust_vars[i] == INCREASE) duty[i] += steps[i];
      if (*adjust_vars[i] == DECREASE) duty[i] -= steps[i];

      duty[i] = duty[i] < duty_min[i] ? duty_min[i] : duty[i];
      duty[i] = duty[i] > duty_max[i] ? duty_max[i] : duty[i];

      if (*adjust_vars[i] != NEUTRAL) Servo_SetDutyCycle(i, duty[i]);
    }
  }
}

void app_main(void) {
  xTaskCreate(ADC_Task, "ADC_Task", 4096, NULL, 10, &adcTaskHandle);
  xTaskCreate(Servo_Task, "Servo_Task", 4096, NULL, 10, &servoTaskHandle);
}
