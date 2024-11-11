/*
 * adc.h
 *
 *  Created on: Aug 25, 2024
 *      Author: iberrios
 */

#ifndef ADC_DMA_H_
#define ADC_DMA_H_

#include <stdint.h>
#include "stm32f4xx.h"

#define NUM_CHANNELS    3 // 6
#define SYS_CLOCK       16000000 // Hz
#define ADC_T_STAB      20 // ADC Stabilization Time in microseconds

// extern uint16_t adc_raw_data[NUM_CHANNELS];


// init ADC1 and DMA2 Stream0 channel 0
void adc_dma_init(void);
void adc_dma_init_6channel(void);


#endif /* ADC_DMA_H_ */
