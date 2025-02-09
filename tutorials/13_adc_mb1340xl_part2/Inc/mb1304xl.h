/*
 * mb1304xl.h
 *
 *  Created on: Oct 24, 2024
 *      Author: iberrios
 */

#ifndef MB1304XL_H_
#define MB1304XL_H_

#include "stm32f4xx.h"

/**
 * Note on Ranging modes
 * Compute sampling time: Num Conversions / Sample Rate
 *
 * Long 0: low res, low update rate, sample time: 0.035 seconds, range = 6m
 * 	- SONAR_TRIGGER_ARR   20000 // 200 ms / 5 Hz PRI
 * 	- SAMPLE_TRIGGER_PW   2200
 * 	- ADC_TIMER_ARR       350   // 2857 Hz Sample Rate
 * 	- NUM_CONVERSIONS     100
 * Long 1: low res, low update rate, sample time: 0.035 seconds, range = 6m
 * 	- SONAR_TRIGGER_ARR   18000 // 180 ms / 5.5 Hz PRI
 * 	- SAMPLE_TRIGGER_PW   2200
 * 	- ADC_TIMER_ARR       350   // 2500 Hz Sample Rate
 * 	- NUM_CONVERSIONS     100
 *
  * Medium 0: tradeoff (range: 3.85m)
 *  - SONAR_TRIGGER_ARR   14000
 *  - SAMPLE_TRIGGER_PW   2200
 *  - ADC_TIMER_ARR       250
 *  - NUM_CONVERSIONS     90
 * Medium 1: tradeoff (range: 3.02m)
 *  - SONAR_TRIGGER_ARR   14000
 *  - SAMPLE_TRIGGER_PW   2200
 *  - ADC_TIMER_ARR       200
 *  - NUM_CONVERSIONS     88
 * Medium 2: tradeoff (range: 2.74m)
 *  - SONAR_TRIGGER_ARR   12500
 *  - SAMPLE_TRIGGER_PW   2200
 *  - ADC_TIMER_ARR       200
 *  - NUM_CONVERSIONS     80
 *
 * Short: high res, high update rate, range = 1.09m
 *  - SONAR_TRIGGER_ARR   10000 // max per spec
 *  - SAMPLE_TRIGGER_PW   2200
 *  - ADC_TIMER_ARR       100   //
 *  - NUM_CONVERSIONS     64   //
 *
* Short: high res, high update rate, range = 0.6m
 *  - SONAR_TRIGGER_ARR   10000 // max per spec
 *  - SAMPLE_TRIGGER_PW   2200
 *  - ADC_TIMER_ARR       50   //
 *  - NUM_CONVERSIONS     70   //
 *
 */
/******************************************************************************************/

/* 'raw' timer configs (need to subtract one in timer config functions) */
#define SONAR_TRIGGER_PSC    160 // slow clock to 0.1MHz --> 16 MHz/160
// #define SONAR_TRIGGER_ARR    10000 // 100ms --> 0.1MHz * 100ms // TEMP for slow testing --> triggers every 3 cycles

/* ADC settings */
// #define ADC_TIMER_PSC       16     // 16MHz/PSC = 1MHz
// #define ADC_TIMER_ARR       500  // 100  // ARR / 1MHz = 0.5 ms rate (don't make this too fast for the ADC
// #define NUM_ADC_CHANNELS    100  // 1000  // 100 seems to work, why?
#define SYS_CLOCK           16000000 // Hz
#define ADC_T_STAB          20       // ADC Stabilization Time in microseconds

//** the above parameters work ok, let's tune some better ones here **/
#define SONAR_TRIGGER_ARR   18000 // 15000 // 150ms --> ARR / 0.1MHz
#define SAMPLE_TRIGGER_PW   2090 // 2200 // 2200  // Sample trigger timer Pulse Width in sonar clocks

#define ADC_TIMER_PSC       16   // 16MHz/PSC = 1MHz
#define ADC_TIMER_ARR       400 // 300 // 120 // 200 // // ARR / 1MHz = 0.5 ms rate (don't make this too fast for the ADC

#define NUM_ADC_CHANNELS    2
#define NUM_CONVERSIONS     80



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
void config_PWM_TIM3_ch1_trigger(void); // adc sample trigger
void config_TIM2_ch4_adc_trigger(void); // adc conversion trigger

/* ADC configs */
void config_adc1_dma(void); // configs ADC1 for 2 channels

/* pin configs */
void config_PA0_AF2(void);    // sonar trigger pin
void config_PA3_AF1(void);    // adc conversion trigger pin (debug)
void config_PA6_AF2(void);    // sample trigger pin (debug)


#endif /* MB1304XL_H_ */
