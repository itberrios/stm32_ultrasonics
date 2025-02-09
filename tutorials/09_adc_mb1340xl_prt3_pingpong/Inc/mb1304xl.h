/*
 * mb1304xl.h
 *
 *  Created on: Oct 24, 2024
 *      Author: iberrios
 */

#ifndef MB1304XL_H_
#define MB1304XL_H_

#include "stm32f4xx.h"


/* 'raw' timer configs (need to subtract one in timer config functions) */
#define SONAR_TRIGGER_PSC    160 // slow clock to 0.1MHz --> 16 MHz/160

/* ADC settings */
#define SYS_CLOCK           16000000 // Hz
#define ADC_T_STAB          20       // ADC Stabilization Time in microseconds

//** the above parameters work ok, let's tune some better ones here **/
#define SONAR_TRIGGER_ARR   10000 // 100ms (10Hz PRI) --> ARR / 0.1MHz

// time to sample after sonar trigger event
#define SAMPLE_TRIGGER_PW   2200 // Sample trigger timer Pulse Width in sonar clocks

#define ADC_TIMER_PSC       16 // 16MHz/PSC = 1MHz (KEEP THIS CONSTANT)
#define ADC_TIMER_ARR       35 // ARR / 1MHz = 35us --> ADC Sample COnversion Rate (don't make this too fast for the ADC)

#define NUM_ADC_CHANNELS    2
#define NUM_CONVERSIONS     1000

#define ADC_BUFFER_SIZE     (NUM_ADC_CHANNELS * NUM_CONVERSIONS)

#define SOUND_METERS_PER_S     343
#define SOUND_METERS_PER_US    0.000343f


/*****************************************************************************************/
/** set Power Supply level (either 3.3V or 5V)
 * Should always be 3.3 since ADC can't handle 5V **/
#define VCC     33 // 33 or 5

/*****************************************************************************************
 ** globals **/
 // declare ping and pong data bufferss
extern uint16_t adc_data_buffer[ADC_BUFFER_SIZE];
extern uint16_t adc_data_buffer_ping[ADC_BUFFER_SIZE];
extern uint16_t adc_data_buffer_pong[ADC_BUFFER_SIZE];

// ADC voltage conversions
extern const float SENSOR_VOLTAGE_SCALE;
extern const float ADC_VOLTAGE_SCALE;
extern const float SAMPLES_TO_CM;

/*****************************************************************************************
 ** configs and protoypes **/

/* timer configs */
void config_PWM_TIM5_ch1_trigger(void); // sensor transmit trigger (PA0)
void config_PWM_TIM3_ch1_trigger(void); // adc sample trigger
void config_TIM2_ch4_adc_trigger(void); // adc conversion trigger

/* ADC configs */
void config_adc1_dma(void); // configs ADC1 for 2 channels

/* pin configs */
void config_PA0_AF2(void);    // sonar trigger pin
void config_PA3_AF1(void);    // adc conversion trigger pin (debug)
void config_PA6_AF2(void);    // sample trigger pin (debug)


#endif /* MB1304XL_H_ */
