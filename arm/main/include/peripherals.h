
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
#define ADC_ATTEN                   ADC_ATTEN_DB_11
#define ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH
#define ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define ADC_GET_CHANNEL(p_data)     ((p_data)->type1.channel)
#define ADC_GET_DATA(p_data)        ((p_data)->type1.data)
#define READ_LEN                    256

#define JOYSTICK_BASE_CHAN      ADC_CHANNEL_0 // 36
#define JOYSTICK_CLAW_CHAN      ADC_CHANNEL_6 // 34
#define JOYSTICK_VERTICAL_CHAN  ADC_CHANNEL_4 // 32
#define JOYSTICK_FORWARD_CHAN   ADC_CHANNEL_5 // 33

#define ADC_COUNT 4

#define JOYSTICK_NEUTRAL_THRESHOLD_MIN 200
#define JOYSTICK_NEUTRAL_THRESHOLD_MAX 3412
  
extern adc_continuous_handle_t adc_handle;
extern const adc_channel_t adc_channels[ADC_COUNT];
 
void ADC_Init();

/* █▀ █▀▀ █▀█ █░█ █▀█ */
/* ▄█ ██▄ █▀▄ ▀▄▀ █▄█ */

#define SERVO_BASE_PIN     GPIO_NUM_23
#define SERVO_CLAW_PIN     GPIO_NUM_3
#define SERVO_VERTICAL_PIN GPIO_NUM_5
#define SERVO_FORWARD_PIN  GPIO_NUM_16

#define SERVO_BASE_NUM     0
#define SERVO_CLAW_NUM     1
#define SERVO_VERTICAL_NUM 2
#define SERVO_FORWARD_NUM  3

#define SERVO_COUNT 4

extern const int servo_pins[SERVO_COUNT];

void Servo_Init();
void Servo_SetDutyCycle(int servo_num, int angle);

#endif
