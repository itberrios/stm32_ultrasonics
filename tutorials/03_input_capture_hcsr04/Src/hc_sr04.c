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
 *
 */

#include "hc_sr04.h"





/* Setup PWM timer on TIM1 channel 1 for ranging trigger
 * desired PW is 10us with a desired PRI of 0.655 seconds
 */
void config_PWM_TIM2_ch1_trigger(void)
{

	// enable clock access to TIM2
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	// ensure TIM2 is disabled
	TIM2->CR1 &= ~TIM_CR1_CEN;

	// set counting direction
	TIM2->CR1 &= ~TIM_CR1_DIR;

	// set prescaler
	TIM2->PSC = 160; // slow clock to 0.1MHz

	// set Auto Reload Value, sets the PRI
	// TIM2->ARR = 5999; // 60ms;
	TIM2->ARR = 7000 - 1; // 70ms

	// set output compare register for channel 1
	TIM2->CCR1 = 1; // 10us, use larger value for larger duty cycle

	// select PWM mode for channel 1
	TIM2->CCMR1 &= ~TIM_CCMR1_OC1M_Msk; // clear bits
	TIM2->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2); // 110 - PWM mode 1

	// enable register preload
	TIM2->CCMR1 |= TIM_CCMR1_OC1PE;

	// slect output polarity to 0 - active high
	TIM2->CCER &= ~TIM_CCER_CC1P;

	// enable output of channel 1
	TIM2->CCER |= TIM_CCER_CC1E;

	// enable all outputs (advanced timers only)
	// TIM1->BDTR |= TIM_BDTR_MOE;

	// enable TIM2
	TIM2->CR1 |= TIM_CR1_CEN;

}


/* configure TIM5 channel 2 for input capture
 *
 */
void config_TIM5_ch2_echo(void)
{
	/** config TIM2, channel 1 for input compare **/
	RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;

	// set Pre-Scaler
//	TIM5->PSC = 16; // slow clock to clock to 1 MHz
	// TIM5->PSC = 0; // don't slow clock, this provides the best measurement resolution!
	// seems like we need to slow the clock down a bit?
	TIM5->PSC = 16 - 1;

	// set ARR to determine sampling period??
	// TIM5->ARR = 59999; // 60ms
	TIM5->ARR = 70000 - 1; // 70ms
	// TIM5->ARR = 0xFFFF;

	// select channel 2 as the active input
	TIM5->CCMR1 &= ~TIM_CCMR1_CC2S_Msk; // clear bits
	TIM5->CCMR1 |= TIM_CCMR1_CC2S_0;    // 01 - maps ch2 to TI2 --> seems to work

	// setup input capture filtering
	TIM5->CCMR1 &= ~TIM_CCMR1_IC2F_Msk; // clear bits
	// TIM5->CCMR1 |= TIM_CCMR1_IC2F_1;    // sample at full timer clock freq, N=4 events
	// TIM5->CCMR1 |= TIM_CCMR1_IC2F_2;
	// TIM5->CCMR1 |= TIM_CCMR1_IC2F_Msk;

	// select the edge of transition
	// TIM5->CCER &= ~(TIM_CCER_CC2P | TIM_CCER_CC2NP); // clear bits, rising edge only
	TIM5->CCER |= TIM_CCER_CC2P | TIM_CCER_CC2NP; // both edges generate interrupts

	// set input Pre-Scaler
	TIM5->CCMR1 &= ~TIM_CCMR1_IC2PSC_Msk; // clear bits

	// Enable capture from the counter for channel 2
	TIM5->CCER |= TIM_CCER_CC2E;

	// enable interrupt on channel 2
	TIM5->DIER |= TIM_DIER_CC2IE;

	// enable DMA request for channel 1 (Optional: DMA only)
	TIM5->DIER |= TIM_DIER_CC2DE;

	// enable TIM5 counter
	TIM5->CR1 |= TIM_CR1_CEN;

	// set interrupt priority
	NVIC_SetPriority(TIM5_IRQn, 0); // highest priority

	// enable TIM5 interrupt on NVIC
	NVIC_EnableIRQ(TIM5_IRQn);

}


/** configure PA0 for Timer 2 channel 1 usage**/
void config_PA0_AF1(void)
{
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// enable Alternate Function mode for PA8
	GPIOA->MODER &= ~GPIO_MODER_MODE0_Msk; // clear Mode bits
	GPIOA->MODER |= GPIO_MODER_MODE0_1;    // set mode bits to 10 for AF

	// set PA8 to AF1 for TIM1
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL0_Msk; // clear AFRL bits
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL0_0;     // set AFRL8 to 0001 for AF1

	// set Output Speed to low
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR0;

	// ensure PA0 has no pull-up or pull-down
	GPIOA->PUPDR &= GPIO_PUPDR_PUPD0_Msk;
}


/** configure PA1 for Timer 5 channel 2 usage**/
void config_PA1_AF2(void)
{
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// enable Alternate Function mode for PA8
	GPIOA->MODER &= ~GPIO_MODER_MODE1_Msk; // clear Mode bits
	GPIOA->MODER |= GPIO_MODER_MODE1_1;    // set mode bits to 10 for AF

	// set PA8 to AF2 for TIM1
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL1_Msk; // clear AFRL bits
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL1_1;     // set AFRL8 to 0010 for AF1

	// set Output Speed to low
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR1;

	// ensure PA0 has no pull-up or pull-down
	GPIOA->PUPDR &= GPIO_PUPDR_PUPD1_Msk;
}
