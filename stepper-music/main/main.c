
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

#include "midi.h"

#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"
#include "sdkconfig.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

static const char *TAG = "example";

#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES   LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY  (440) // Hz
#define LEDC_DUTY       4095

#define LOW  0
#define HIGH 1  

// Frequencies
// these note names are wrong
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

  // LEDC channel configuration
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
  ledc_set_freq(LEDC_MODE, LEDC_TIMER_0, A1);
  while (1) { vTaskDelay(100); }
}

TaskHandle_t Stepper1_TaskHandle = NULL;
void Stepper1_Task(void * arg)
{
  Stepper_Init(STEPPER_1_PIN, LEDC_CHANNEL_1, LEDC_TIMER_1);
  ledc_set_freq(LEDC_MODE, LEDC_TIMER_1, A1);
  while (1) { vTaskDelay(100); }
}

TaskHandle_t Stepper2_TaskHandle = NULL;
void Stepper2_Task(void * arg)
{
  Stepper_Init(STEPPER_2_PIN, LEDC_CHANNEL_2, LEDC_TIMER_2);
  ledc_set_freq(LEDC_MODE, LEDC_TIMER_2, A1);
  while (1) {
    vTaskDelay(100);
    printf("task2. wat\n");
  }
}

void app_main(void)
{
  // Mount filesystem
  const char *base_path = "/spiflash";

  const esp_vfs_fat_mount_config_t mount_config = {
    .max_files = 4,
    .format_if_mount_failed = false,
    .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
  };

  esp_err_t err;
  err = esp_vfs_fat_spiflash_mount_ro(base_path, "storage", &mount_config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
    return;
  }

  FILE* f;
  char* filename = "/spiflash/test.mid";

  // Get file details
  struct stat st;
  if(stat(filename, &st) < 0){
    ESP_LOGE(TAG, "Failed to read file stats");
    return;
  }

  // Open file
  f = fopen(filename, "rb");
  if (f == NULL) {
    ESP_LOGE(TAG, "Failed to open file for reading");
    return;
  }

  // Read bytes from file
  char c;
  while (1) {
    c = fgetc(f);
    if (c == EOF) break;
    printf("%x\n", c);
    vTaskDelay(10);
  }

  // Close and unmount
  fclose(f);
  esp_vfs_fat_spiflash_unmount_ro(base_path, "storage");

  // xTaskCreate(Stepper0_Task, "Stepper0_Task", 4096, NULL, 10, &Stepper0_TaskHandle);
  // xTaskCreate(Stepper1_Task, "Stepper1_Task", 4096, NULL, 10, &Stepper1_TaskHandle);
  // xTaskCreate(Stepper2_Task, "Stepper2_Task", 4096, NULL, 10, &Stepper2_TaskHandle);
}
