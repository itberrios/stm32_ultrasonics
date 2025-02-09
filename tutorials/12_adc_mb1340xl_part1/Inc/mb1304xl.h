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
#define SONAR_TRIGGER_PSC    160 // slow clock to 0.1MHz --> 16 MHz/160
#define SONAR_TRIGGER_ARR    10000 // 100ms --> ARR / 0.1MHz

/* ADC settings */
#define SYS_CLOCK           16000000 // Hz
#define ADC_T_STAB          20       // ADC Stabilization Time in microseconds
#define NUM_ADC_CHANNELS    2
#define NUM_CONVERSIONS     10

#define ADC_BUFFER_SIZE     (NUM_ADC_CHANNELS * NUM_CONVERSIONS)

/*****************************************************************************************/
/** set Power Supply level (either 3.3V or 5V)
 * Should always be 3.3 since ADC can't handle 5V **/
#define VCC     33 // 33 or 5

/*****************************************************************************************
 ** globals **/
 // store ADC data in buffer
extern uint16_t adc_data_buffer[ADC_BUFFER_SIZE];

// ADC voltage conversions
extern const float SENSOR_VOLTAGE_SCALE;
extern const float ADC_VOLTAGE_SCALE;
extern const float SAMPLES_TO_CM;

/*****************************************************************************************
 ** configs and protoypes **/

/* timer configs */
void config_PWM_TIM5_ch1_trigger(void); // sensor transmit trigger (PA0)

/* ADC configs */
void config_adc1_dma(void); // configs ADC1 for 2 channels

/* pin configs */
void config_PA0_AF2(void);    // sonar trigger pin


#endif /* MB1304XL_H_ */
