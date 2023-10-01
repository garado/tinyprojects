
#ifndef __PERIPHERALS_H__
#define __PERIPHERALS_H__

#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_continuous.h"

/* ▄▀█ █▀▄ █▀▀ */ 
/* █▀█ █▄▀ █▄▄ */ 

#define ADC_UNIT                    ADC_UNIT_1
#define ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1
#define ADC_ATTEN                   ADC_ATTEN_DB_0
#define ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH
#define ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define ADC_GET_CHANNEL(p_data)     ((p_data)->type1.channel)
#define ADC_GET_DATA(p_data)        ((p_data)->type1.data)
#define READ_LEN                    256

#define JOYSTICK_VRX_CHAN ADC_CHANNEL_6
#define JOYSTICK_VRY_CHAN ADC_CHANNEL_7
#define JOYSTICK_NEUTRAL_THRESHOLD_MIN 3000
#define JOYSTICK_NEUTRAL_THRESHOLD_MAX 4000
  
extern adc_continuous_handle_t adc_handle;
extern const adc_channel_t adc_channels[2];
 
void ADC_Init();

#endif
