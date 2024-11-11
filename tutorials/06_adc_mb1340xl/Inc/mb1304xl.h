/*
 * mb1304xl.h
 *
 *  Created on: Oct 24, 2024
 *      Author: iberrios
 */

#ifndef MB1304XL_H_
#define MB1304XL_H_

#include "stm32f4xx.h"


// 'raw' timer configs (need to subtract one in timer config functions)
#define TRIGGER_TIMER_PSC    160 // slow clock to 0.1MHz --> 16 MHz/160
// #define TRIGGER_TIMER_ARR    7000 // 70ms --> 0.1MHz * 70ms
#define TRIGGER_TIMER_ARR    70000 // 700ms --> 0.1MHz * 700ms // TEMP for slow testing

// ADC settings
#define NUM_ADC_CHANNELS    2
#define SYS_CLOCK           16000000 // Hz
#define ADC_T_STAB          20       // ADC Stabilization Time in microseconds

extern uint16_t adc_data[NUM_ADC_CHANNELS];


/* timer configs */
void config_PWM_TIM2_ch1_trigger(void);

/* ADC configs */
void config_adc1_dma(void); // configs ADC1 for 2 channels

/* pin configs */
void config_PA0_AF1(void);    // trigger pin
void config_PA1_ADC1_1(void); // analog voltage envelope (Pin 2)
void config_PA2_ADC1_2(void); // range voltage level (Pin 3) (needs 10bit conversion)



#endif /* MB1304XL_H_ */
