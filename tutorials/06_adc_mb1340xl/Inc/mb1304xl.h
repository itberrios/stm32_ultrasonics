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
#define TRIGGER_TIMER_PSC    160 // slow clock to 0.1MHz (10us) --> 16 MHz/160
// #define TRIGGER_TIMER_ARR    7000 // 70ms --> 0.1MHz * 70ms

/* TEMP for slow testing
 * NOTES:
 * 	- the 100ms PRI ensures that the timer counts up to 10000
 * 	- it looks like the Pulse goes high around 2200?
 */
#define TRIGGER_TIMER_ARR    10000 // 100ms --> 0.1MHz * 100ms

/* ADC settings */
#define NUM_ADC_CHANNELS    2
#define NUM_ADC_SAMPLES     100 // use this to sample a bunch of stuff at once
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

// ADC voltage conversions
extern const float SENSOR_VOLTAGE_SCALE;
extern const float ADC_VOLTAGE_SCALE;
extern const float SAMPLES_TO_CM;

/*****************************************************************************************
 ** configs and protoypes **/

/* timer configs */
void config_PWM_TIM2_ch1_trigger(void);

/* ADC configs */
void config_adc1_dma(void); // configs ADC1 for 2 channels

/* pin configs */
void config_PA0_AF1(void);    // trigger pin
void config_PA1_ADC1_1(void); // analog voltage envelope (Pin 2)
void config_PA2_ADC1_2(void); // range voltage level (Pin 3) (needs 10bit conversion)



#endif /* MB1304XL_H_ */
