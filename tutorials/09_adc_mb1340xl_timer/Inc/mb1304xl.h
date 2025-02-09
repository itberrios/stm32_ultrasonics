/*
 * mb1304xl.h
 *
 *  Created on: Oct 24, 2024
 *      Author: iberrios
 */

#ifndef MB1304XL_H_
#define MB1304XL_H_

#include "stm32f4xx.h"

/******************************************************************************************/

/* 'raw' timer configs (need to subtract one in timer config functions) */
#define TRIGGER_TIMER_PSC    160 // slow clock to 0.1MHz --> 16 MHz/160
// #define TRIGGER_TIMER_ARR    7000 // 70ms --> 0.1MHz * 70ms --> triggers every 2 cycles
// #define TRIGGER_TIMER_ARR    1000 // 100ms --> 0.1MHz * 100ms
// #define TRIGGER_TIMER_ARR    70000 // 700ms --> 0.1MHz * 700ms // TEMP for slow testing --> seems to trigger on every cycle??
#define TRIGGER_TIMER_ARR    10000 // 100ms --> 0.1MHz * 100ms // TEMP for slow testing --> triggers every 3 cycles

/* ADC settings */
#define ADC_TIMER_PSC       16     // 16MHz/PSC = 1MHz
#define ADC_TIMER_ARR       500   // ARR / 1MHz = 0.5 ms rate (don't make this too fast for the ADC
#define NUM_ADC_CHANNELS    2 // 160
#define NUM_CONVERSIONS     1000
#define SYS_CLOCK           16000000 // Hz
#define ADC_T_STAB          20       // ADC Stabilization Time in microseconds

/*****************************************************************************************/
/** set Power Supply level (either 3.3V or 5V)
 * Should always be 3.3 since ADC can't handle 5V **/
#define VCC     33 // 33 or 5

/*****************************************************************************************
 ** globals **/
 // store ADC data
extern uint16_t adc_data[NUM_ADC_CHANNELS];
extern uint16_t envelope_buffer[100];

// ADC voltage conversions
extern const float SENSOR_VOLTAGE_SCALE;
extern const float ADC_VOLTAGE_SCALE;
extern const float SAMPLES_TO_CM;

/*****************************************************************************************
 ** configs and protoypes **/

/* timer configs */
void config_PWM_TIM2_ch1_trigger(void); // PA0
void config_PWM_TIM5_ch1_trigger(void); // PA0
//void config_PWM_TIM2_ch4_trigger(void); // PA3
void config_PWM_TIM2_ch4_adc_trigger(void);

/* ADC configs */
void config_adc1_dma(void); // configs ADC1 for 2 channels

void config_DMA2_mem2mem(void);

/* pin configs */
void config_PA0_AF1(void);    // trigger pin
void config_PA0_AF2(void);    // trigger pin
void config_PA3_AF1(void);    // trigger pin
void config_PA1_ADC1_1(void); // analog voltage envelope (Pin 2)
void config_PA2_ADC1_2(void); // range voltage level (Pin 3) (needs 10bit conversion)



#endif /* MB1304XL_H_ */
