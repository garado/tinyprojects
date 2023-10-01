
/* █▀█ █▀█ █▄▄ █▀█ ▀█▀    ▄▀█ █▀█ █▀▄▀█ */
/* █▀▄ █▄█ █▄█ █▄█ ░█░    █▀█ █▀▄ █░▀░█ */

// Pins used
// GPIO34 (ADC Unit 1 Ch6) - Joystick Vrx (yellow)
// GPIO35 (ADC Unit 1 Ch7) - Joystick Vry (white)

#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_continuous.h"
#include "peripherals.h"

TaskHandle_t adcTaskHandle = NULL;

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

        if (chan_num == JOYSTICK_VRX_CHAN) {
          if (data < JOYSTICK_NEUTRAL_THRESHOLD_MIN) printf("LEFT\n");
          if (data > JOYSTICK_NEUTRAL_THRESHOLD_MAX) printf("RIGHT\n");
        } else if (chan_num == JOYSTICK_VRY_CHAN) {
          if (data < JOYSTICK_NEUTRAL_THRESHOLD_MIN) printf("UP\n");
          if (data > JOYSTICK_NEUTRAL_THRESHOLD_MAX) printf("DOWN\n");
        }
      }
    }
  }
}

void app_main(void) {
  xTaskCreate(ADC_Task, "ADC_Task", 4096, NULL, 10, &adcTaskHandle);
}
