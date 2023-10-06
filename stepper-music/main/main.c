
#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "soc/gpio_reg.h"

#include "midi.h"
#include "frequency.h"
#include "midi_god.h"

#define LOW  0
#define HIGH 1
#define STEPPER_COUNT 4

// Pin definitions
#define STEPPER_0_PIN GPIO_NUM_2
#define STEPPER_1_PIN GPIO_NUM_1
#define STEPPER_2_PIN GPIO_NUM_21
#define STEPPER_3_PIN GPIO_NUM_18
#define STEPPER_4_PIN GPIO_NUM_17
#define GPIO_OUT_PINS ((1ULL << STEPPER_0_PIN) | (1ULL << STEPPER_1_PIN)| (1ULL << STEPPER_2_PIN)| \
                       (1ULL << STEPPER_3_PIN)| (1ULL << STEPPER_4_PIN))

static const char * TAG = "STEPPER";
TaskHandle_t Stepper0_TaskHandle = NULL;

static bool IRAM_ATTR timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
  BaseType_t must_yield = pdFALSE;
  vTaskNotifyGiveFromISR(Stepper0_TaskHandle, &must_yield);
  return (must_yield == pdTRUE);
}

void Stepper0_Task(void * arg)
{
  while (1) {
    xTaskNotifyWait(0, 0, 0, portMAX_DELAY);
    gpio_set_level(STEPPER_0_PIN, HIGH);
    xTaskNotifyWait(0, 0, 0, portMAX_DELAY);
    gpio_set_level(STEPPER_0_PIN, LOW);
  }
}

void app_main()
{
  // GPIO init
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = GPIO_OUT_PINS;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  // Create timer handle
  gptimer_handle_t gptimer = NULL;
  gptimer_config_t timer_config = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz = 1000000, // 1MHz, 1 tick=1us
  };
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

  gptimer_event_callbacks_t cbs = { .on_alarm = timer_callback };
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

  // Set up tasks
  xTaskCreate(Stepper0_Task, "Stepper0_Task", 2048, NULL, 10, &Stepper0_TaskHandle);

  // Enable and start timer
  ESP_ERROR_CHECK(gptimer_enable(gptimer));
  gptimer_alarm_config_t alarm_config1 = {
    .alarm_count = 1000000, // period = 1s
    .reload_count = 0,
    .flags.auto_reload_on_alarm = true,
  };
  ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config1));
  ESP_ERROR_CHECK(gptimer_start(gptimer));
}
