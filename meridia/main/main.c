
/* █▀▄▀█ █▀▀ █▀█ █ █▀▄ █ ▄▀█ █  █▀    █▄▄ █▀▀ ▄▀█ █▀▀ █▀█ █▄░█ 
 * █░▀░█ ██▄ █▀▄ █ █▄▀ █ █▀█    ▄█    █▄█ ██▄ █▀█ █▄▄ █▄█ █░▀█ 
 */

#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/adc.h"
#include "driver/dac_continuous.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "audio.h"

#define LOW  0
#define HIGH 1
#define TRIGGER_PIN GPIO_NUM_23
#define GPIO_INPUT_PIN_SEL (1ULL << TRIGGER_PIN)

void app_main(void)
{
  // Configure trigger pin
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_POSEDGE;
  io_conf.pin_bit_mask = 1ULL << TRIGGER_PIN;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  // Configure DAC
  dac_continuous_config_t cont_cfg = {
    .chan_mask = DAC_CHANNEL_MASK_ALL,
    .desc_num = 4,
    .buf_size = 2048,
    .freq_hz = CONFIG_EXAMPLE_AUDIO_SAMPLE_RATE,
    .offset = 0,
    .clk_src = DAC_DIGI_CLK_SRC_APLL,  // Using APLL as clock source to get a wider frequency range
    .chan_mode = DAC_CHANNEL_MODE_SIMUL,
  };

  dac_continuous_handle_t dac_handle;
  dac_continuous_new_channels(&cont_cfg, &dac_handle);
  dac_continuous_enable(dac_handle);

  uint8_t current_state = gpio_get_level(TRIGGER_PIN);
  uint8_t last_state = current_state;
  const size_t audio_size = sizeof(audio_table);

  while(1) {
    current_state = gpio_get_level(TRIGGER_PIN);

    if ((last_state != current_state) && current_state == HIGH) {
      dac_continuous_write(dac_handle, (uint8_t *)audio_table, audio_size, NULL, -1);
    }

    last_state = current_state;
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

