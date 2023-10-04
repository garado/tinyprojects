
#include <esp_log.h>
#include <inttypes.h>
#include "rc522.h"

#define PIN_SPI_SDA  5
#define PIN_SPI_SCK  18
#define PIN_SPI_MOSI 23
#define PIN_SPI_MISO 19

static const char* TAG = "rc522-demo";
static rc522_handle_t scanner;

static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
  rc522_event_data_t* data = (rc522_event_data_t*) event_data;

  switch(event_id) {
    case RC522_EVENT_TAG_SCANNED: {
      rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
      ESP_LOGI(TAG, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
      break;
    }
    default: break;
  }
}

void app_main()
{
  rc522_config_t config = {
    .spi.host = VSPI_HOST,
    .spi.miso_gpio = PIN_SPI_MISO,
    .spi.mosi_gpio = PIN_SPI_MOSI,
    .spi.sck_gpio  = PIN_SPI_SCK,
    .spi.sda_gpio  = PIN_SPI_SDA,
  };

  rc522_create(&config, &scanner);
  rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
  rc522_start(scanner);
}
