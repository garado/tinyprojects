/*
 * █▀▄▀█ █▀▀ █▀█ █ █▀▄ █ ▄▀█ █  █▀    █▄▄ █▀▀ ▄▀█ █▀▀ █▀█ █▄░█ 
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

// speaker output pin is d26
#define TRIGGER_PIN      GPIO_NUM_23
#define AUDIO_LENGTH_MS  3500

#define GPIO_INPUT_PIN_SEL (1ULL << TRIGGER_PIN)

#define ESP_INTR_FLAG_DEFAULT 0

dac_continuous_handle_t dac_handle;
static QueueHandle_t gpio_evt_queue = NULL;

size_t audio_size = sizeof(audio_table);
uint8_t audio_playing = false;

static void dac_write_data_synchronously(dac_continuous_handle_t handle, uint8_t *data, int data_size)
{
  dac_continuous_write(handle, data, data_size, NULL, -1);
  vTaskDelay(pdMS_TO_TICKS(AUDIO_LENGTH_MS));
  audio_playing = false;
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
  uint32_t gpio_num = (uint32_t) arg;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Poll for trigger neg edge
static void trigger_task(void* arg)
{
  uint32_t io_num;
  while(1) {
    if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
      if (!audio_playing) {
        audio_playing = true;
        dac_write_data_synchronously(dac_handle, (uint8_t *)audio_table, audio_size);
      }
    }
  }
}

void app_main(void)
{
  // Configure trigger pin
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_POSEDGE;
  io_conf.pin_bit_mask = 1ULL << TRIGGER_PIN;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
  xTaskCreate(trigger_task, "trigger_task", 2048, NULL, 10, NULL);

  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  gpio_isr_handler_add(TRIGGER_PIN, gpio_isr_handler, (void*) TRIGGER_PIN);

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

  /* Allocate continuous channels */
  dac_continuous_new_channels(&cont_cfg, &dac_handle);
  dac_continuous_enable(dac_handle);
}
