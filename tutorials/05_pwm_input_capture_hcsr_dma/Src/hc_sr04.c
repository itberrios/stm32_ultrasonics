/*
 * hc_sr04.c
 *
 *  Created on: Oct 6, 2024
 *      Author: iberrios
 *
 *  control the HC-SR04 ultrasonic range finder for the STM32F411RE
 *
 *  Coding style notes:
 *  	'_config' prefixes indicate that the program is configuring things on
 *  	    the microcontroller. These functions shouldn't be exposed in the
 *  	    header file and are for internal usage. Instead use the 'init' prefix
 *  	    since it describes a desired initialization of some sort of functionality
 *
 *  	'init' prefixes are functions that initialize some desired functionality
 *  	i.e. initialize PA0 for PWM output, this involves many configurations
 *  	which do not need to be exposed outside of this header. Since these
 *  	init functions are usually called once, it might be acceptable to introduce
 *  	some extra overhead from multiple functions for the tradeoff of maintainablility
 *
 *	Need to explicitly clear all the registers, probably need to use the STM32 header code to bit bang
 *
 */

#include "hc_sr04.h"

/* globals */
uint32_t tim5_channel1_ccr[1] = {0}; // clocks at rising edge
uint32_t tim5_channel2_ccr[1] = {0}; // clocks at falling edge

/* prototypes */
void config_dma1_ch6_stream2(void);
void config_dma1_ch6_stream4(void);


/* Setup PWM timer on TIM1 channel 2 for ranging trigger
 * desired PW is 10us with a desired PRI of 0.7 seconds
 */
void config_PWM_TIM2_ch2_trigger(void)
{
	// enable clock access to TIM2
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	// ensure TIM2 is disabled
	TIM2->CR1 &= ~TIM_CR1_CEN;

	// set counting direction
	TIM2->CR1 &= ~TIM_CR1_DIR;

	// set prescaler
	TIM2->PSC = TRIGGER_TIMER_PSC;

	// set Auto Reload Value, sets the PRI
	TIM2->ARR = TRIGGER_TIMER_ARR - 1;

	// set output compare register for channel 2
	TIM2->CCR2 = 1; // 10us, use larger value for larger duty cycle

	// select PWM mode for channel 2
	TIM2->CCMR1 &= ~TIM_CCMR1_OC2M_Msk; // clear bits
	TIM2->CCMR1 |= (TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2); // 110 - PWM mode 1

	// enable register preload
	TIM2->CCMR1 |= TIM_CCMR1_OC2PE;

	// slect output polarity to 0 - active high
	TIM2->CCER &= ~TIM_CCER_CC2P;

	// enable output of channel 2
	TIM2->CCER |= TIM_CCER_CC2E;

	// enable TIM2
	TIM2->CR1 |= TIM_CR1_CEN;

}



/* configure TIM5 channel 2 for basic pwm input capture
 *
 */
void config_TIM5_ch1_echo_pwm_input(void)
{
	/** config TIM2, channel 1 for input compare **/
	RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;

	// set Pre-Scaler
	TIM5->PSC = ECHO_TIMER_PSC - 1;

	// set ARR to determine sampling period
	TIM5->ARR = ECHO_TIMER_ARR - 1;

	// Disable capture from the counter for channels 1 and 2
	TIM5->CCER &= ~TIM_CCER_CC1E;
	TIM5->CCER &= ~TIM_CCER_CC2E;

	/** configure channel 1 for active rising input **/
	// select channel 1 as the active input
	TIM5->CCMR1 &= ~TIM_CCMR1_CC1S_Msk; // clear bits
	TIM5->CCMR1 |= TIM_CCMR1_CC1S_0;    // 01 - maps ch1 to TI1

	// setup input capture filtering
	TIM5->CCMR1 &= ~TIM_CCMR1_IC1F_Msk; // clear bits

	// select the edge of transition
	TIM5->CCER &= ~(TIM_CCER_CC2P | TIM_CCER_CC2NP); // 00 - active rising

	// set input Pre-Scaler
	TIM5->CCMR1 &= ~TIM_CCMR1_IC1PSC_Msk; // clear bits

	/** configure channel 2 for active falling input **/
	// select channel 2 as the active input
	TIM5->CCMR1 &= ~TIM_CCMR1_CC2S_Msk; // clear bits
	TIM5->CCMR1 |= TIM_CCMR1_CC2S_1;    // 10 - maps channel 2 to TI1

	// setup input capture filtering
	TIM5->CCMR1 &= ~TIM_CCMR1_IC2F_Msk; // clear bits

	// select the edge of transition
	TIM5->CCER &= ~(TIM_CCER_CC2P | TIM_CCER_CC2NP); // clear bits
	TIM5->CCER |= TIM_CCER_CC2P; // 01 - active falling

	// set input Pre-Scaler
	TIM5->CCMR1 &= ~TIM_CCMR1_IC2PSC_Msk; // clear bits

	// select valid trigger input
	TIM5->SMCR &= ~TIM_SMCR_TS_Msk; // clear bits
	TIM5->SMCR |= (TIM_SMCR_TS_0 | TIM_SMCR_TS_2); // 101 - Filtered Timer Input 1 (TI1FP1)

	// configure the slave mode controller in reset mode
	TIM5->SMCR &= ~TIM_SMCR_SMS_Msk; // clear bits
	TIM5->SMCR |= TIM_SMCR_SMS_2; // 100 - rising edge resets the counter

	/**  turn stuff on and configure DMA requests **/
	// Enable capture from the counter for channel 1
	TIM5->CCER |= TIM_CCER_CC1E;

	// enable DMA request for channel 1 (Optional: DMA only)
	TIM5->DIER |= TIM_DIER_CC1DE;

	// Enable capture from the counter for channel 2
	TIM5->CCER |= TIM_CCER_CC2E;

	// enable DMA request for channel 2 (Optional: DMA only)
	TIM5->DIER |= TIM_DIER_CC2DE;

	/* Configure DMA controller
	 * DMA ch 6 stream 2 --> TIM5 ch 1
	 * DMA ch 6 stream 4 --> TIM5 ch 2
	 */
	config_dma1_ch6_stream2(); // setup DMA for rising edge capture
	config_dma1_ch6_stream4(); // config DMA for falling edge capture

	// enable TIM5 counter
	TIM5->CR1 |= TIM_CR1_CEN;
}




/* config DMA1 Channel 6, Stream 2 for TIM5 ch 1 */
void config_dma1_ch6_stream2(void)
{
	// enable clock access to DMA module
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

	// reset stream 2
	DMA1_Stream2->CR = 0;
	while((DMA1_Stream2->CR & DMA_SxCR_EN) != 0) {}

	// select channel 6
	DMA1_Stream2->CR &= ~DMA_SxCR_CHSEL_0;
	DMA1_Stream2->CR |= DMA_SxCR_CHSEL_1;
	DMA1_Stream2->CR |= DMA_SxCR_CHSEL_2;

	// enable memory address increment
	// DMA1_Stream2->CR |= DMA_SxCR_MINC;

	// set memory size to 32bit
	DMA1_Stream2->CR &= ~DMA_SxCR_MSIZE_0;
	DMA1_Stream2->CR |= DMA_SxCR_MSIZE_1;

	// set peripheral size to 32 bit
	DMA1_Stream2->CR &= ~DMA_SxCR_PSIZE_0;
	DMA1_Stream2->CR |= DMA_SxCR_PSIZE_1;

	// set transfer direction  (Peripheral-to-Memory)
	DMA1_Stream2->CR &= ~DMA_SxCR_DIR_Msk;

	// enable circular mode
	DMA1_Stream2->CR |= DMA_SxCR_CIRC;

	// Set number of transfers
	DMA1_Stream2->NDTR = (uint16_t) 1;

	// set peripheral address
	DMA1_Stream2->PAR = (uint32_t) (&TIM5->CCR1);

	// set memory address (need to store data in an array? Or variable??)
	DMA1_Stream2->M0AR = (uint32_t) tim5_channel1_ccr;

	// enable transfer complete interrupt
	DMA1_Stream2->CR |= DMA_SxCR_TCIE;

	// enable interrupt on NVIC
	NVIC_EnableIRQ(DMA1_Stream2_IRQn);

	// enable DMA stream
	DMA1_Stream2->CR |= DMA_SxCR_EN;

}


/* config DMA 1 Channel 6, Stream 4 for TIM 5 ch 2 */
void config_dma1_ch6_stream4(void)
{
	// enable clock access to DMA module
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

	// reset stream 4
	DMA1_Stream4->CR = 0;
	while((DMA1_Stream4->CR & DMA_SxCR_EN) != 0) {}

	// select channel 6
	DMA1_Stream4->CR &= ~DMA_SxCR_CHSEL_0;
	DMA1_Stream4->CR |= DMA_SxCR_CHSEL_1;
	DMA1_Stream4->CR |= DMA_SxCR_CHSEL_2;

	// set DMA priority
	DMA1_Stream4->CR |= DMA_SxCR_PL_Msk;

	// enable memory address increment
	// DMA1_Stream4->CR |= DMA_SxCR_MINC;

	// set memory size to 32bit
	DMA1_Stream4->CR &= ~DMA_SxCR_MSIZE_0;
	DMA1_Stream4->CR |= DMA_SxCR_MSIZE_1;

	// set peripheral size to 32 bit
	DMA1_Stream4->CR &= ~DMA_SxCR_PSIZE_0;
	DMA1_Stream4->CR |= DMA_SxCR_PSIZE_1;

	// set transfer direction  (Peripheral-to-Memory)
	DMA1_Stream4->CR &= ~DMA_SxCR_DIR_Msk;

	// enable circular mode
	DMA1_Stream4->CR |= DMA_SxCR_CIRC;

	// Set number of transfers
	DMA1_Stream4->NDTR = (uint16_t) 1;

	// set peripheral address
	DMA1_Stream4->PAR = (uint32_t) (&TIM5->CCR2);

	// set memory address (need to store data in an array? Or variable??)
	DMA1_Stream4->M0AR = (uint32_t) tim5_channel2_ccr;

	// enable transfer complete interrupt
	DMA1_Stream4->CR |= DMA_SxCR_TCIE;

	// enable interrupt on NVIC
	NVIC_EnableIRQ(DMA1_Stream4_IRQn);
	// NVIC_SetPriority(DMA1_Stream4_IRQn, 0); // highest priority

	// enable DMA stream
	DMA1_Stream4->CR |= DMA_SxCR_EN;
}



/** configure PA0 for Timer 5 channel 1 usage**/
void config_PA0_AF2(void)
{
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// enable Alternate Function mode for PA0
	GPIOA->MODER &= ~GPIO_MODER_MODE0_Msk; // clear Mode bits
	GPIOA->MODER |= GPIO_MODER_MODE0_1;    // set mode bits to 10 for AF

	// set PA0 to AF2 for TIM5
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL0_Msk; // clear AFRL bits
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL0_1;     // set AFRL to 0010 for AF2

	// set Output Speed to low
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR0;

	// ensure PA0 has no pull-up or pull-down
	GPIOA->PUPDR &= GPIO_PUPDR_PUPD0_Msk;
}


/** configure PA1 for Timer 2 channel 2 usage**/
void config_PA1_AF1(void)
{
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// enable Alternate Function mode for PA8
	GPIOA->MODER &= ~GPIO_MODER_MODE1_Msk; // clear Mode bits
	GPIOA->MODER |= GPIO_MODER_MODE1_1;    // set mode bits to 10 for AF

	// set PA1 to AF1 for TIM2
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL1_Msk; // clear AFRL bits
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL1_0;     // set AFRL8 to 0001 for AF1

	// set Output Speed to low
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR1;

	// ensure PA0 has no pull-up or pull-down
	GPIOA->PUPDR &= GPIO_PUPDR_PUPD1_Msk;
}
