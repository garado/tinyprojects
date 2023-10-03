
#include "peripherals.h"
#include "driver/ledc.h"
#include "hal/ledc_types.h"

/* ▄▀█ █▀▄ █▀▀ */
/* █▀█ █▄▀ █▄▄ */

adc_continuous_handle_t adc_handle = NULL;
const adc_channel_t adc_channels[ADC_COUNT] = {
  JOYSTICK_BASE_CHAN,
  JOYSTICK_CLAW_CHAN,
  JOYSTICK_VERTICAL_CHAN,
  JOYSTICK_FORWARD_CHAN,
};
 
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

/* █▀ █▀▀ █▀█ █░█ █▀█ */
/* ▄█ ██▄ █▀▄ ▀▄▀ █▄█ */

// Using ledc lib to control servos

#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_DUTY_RES   LEDC_TIMER_13_BIT
#define LEDC_DUTY       (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY  (50)   // Hz

const int servo_pins[SERVO_COUNT] = { 
  SERVO_BASE_PIN,
  SERVO_CLAW_PIN,
  SERVO_VERTICAL_PIN,
  SERVO_FORWARD_PIN,
};

const int servo_channels[SERVO_COUNT] = {
  LEDC_CHANNEL_0,
  LEDC_CHANNEL_1,
  LEDC_CHANNEL_2,
  LEDC_CHANNEL_3
};

void Servo_Init()
{
  // PWM timer configuration
  ledc_timer_config_t ledc_timer = {
    .speed_mode       = LEDC_MODE,
    .timer_num        = LEDC_TIMER,
    .duty_resolution  = LEDC_DUTY_RES,
    .freq_hz          = LEDC_FREQUENCY,
    .clk_cfg          = LEDC_AUTO_CLK
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  // PWM channel configuration
  for (int i = 0; i < SERVO_COUNT; i++) {
    ledc_channel_config_t ledc_channel = {
      .speed_mode     = LEDC_MODE,
      .channel        = servo_channels[i],
      .timer_sel      = LEDC_TIMER,
      .intr_type      = LEDC_INTR_DISABLE,
      .gpio_num       = servo_pins[i],
      .duty           = 4095,
      .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
  }
}

void Servo_SetDutyCycle(int servo_num, int angle)
{
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, servo_channels[servo_num], angle));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, servo_channels[servo_num]));
}
