
#include "peripherals.h"

/* ▄▀█ █▀▄ █▀▀ */
/* █▀█ █▄▀ █▄▄ */

adc_continuous_handle_t adc_handle = NULL;
const adc_channel_t adc_channels[2] = { JOYSTICK_VRX_CHAN, JOYSTICK_VRY_CHAN };
 
/* @function ADC_Init 
 * @brief Initialize ADC for joystick */
void ADC_Init(void)
{
  uint8_t channel_num = sizeof(adc_channels) / sizeof(adc_channel_t);

  adc_handle = NULL;

  adc_continuous_handle_cfg_t adc_config = {
    .max_store_buf_size = 1024,
    .conv_frame_size = READ_LEN,
  };
  ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &adc_handle));

  adc_continuous_config_t dig_cfg = {
    .sample_freq_hz = 20 * 1000,
    .conv_mode = ADC_CONV_MODE,
    .format = ADC_OUTPUT_TYPE,
  };

  adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
  dig_cfg.pattern_num = channel_num;
  for (int i = 0; i < channel_num; i++) {
    adc_pattern[i].atten = ADC_ATTEN;
    adc_pattern[i].channel = adc_channels[i] & 0x7;
    adc_pattern[i].unit = ADC_UNIT;
    adc_pattern[i].bit_width = ADC_BIT_WIDTH;
  }
  dig_cfg.adc_pattern = adc_pattern;
  ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &dig_cfg));
}
