/*
 * mb1304xl.c
 *
 *  Created on: Oct 24, 2024
 *      Author: iberrios
 */


#include "mb1304xl.h"

/* Setup PWM timer on TIM1 channel 1 for ranging trigger
 * desired PW is 50us with a desired PRI of 70ms
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
	TIM2->CCR2 = 5; // 50us?? // 10us, use larger value for larger duty cycle

	// select PWM mode for channel 2
	TIM2->CCMR1 &= ~TIM_CCMR1_OC2M_Msk; // clear bits
	TIM2->CCMR1 |= (TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2); // 110 - PWM mode 1

	// enable register preload
	TIM2->CCMR1 |= TIM_CCMR1_OC2PE;

	// slect output polarity to 0 - active high
	TIM2->CCER &= ~TIM_CCER_CC2P;

	// enable output of channel 1
	TIM2->CCER |= TIM_CCER_CC2E;

	// enable all outputs (advanced timers only)
	// TIM1->BDTR |= TIM_BDTR_MOE;

	// enable TIM2
	TIM2->CR1 |= TIM_CR1_CEN;

}


/* configure TIM5 channel 2 for basic input capture
 *
 */
void config_TIM5_ch1_echo(void)
{
	/** config TIM2, channel 2 for input compare **/
	RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;

	// set Pre-Scaler
	TIM5->PSC = ECHO_TIMER_PSC - 1;

	// set ARR to determine sampling period
	TIM5->ARR = ECHO_TIMER_ARR - 1;

	// Disable capture from the counter for channel 1
	TIM5->CCER &= ~TIM_CCER_CC1E;

	// select channel 1 as the active input
	TIM5->CCMR1 &= ~TIM_CCMR1_CC1S_Msk; // clear bits
	TIM5->CCMR1 |= TIM_CCMR1_CC1S_0;    // 01 - maps ch1 to TI1

	// setup input capture filtering
	TIM5->CCMR1 &= ~TIM_CCMR1_IC1F_Msk; // clear bits

	// select the edge of transition
	// TIM5->CCER &= ~(TIM_CCER_CC2P | TIM_CCER_CC2NP); // clear bits, rising edge only
	TIM5->CCER |= TIM_CCER_CC1P | TIM_CCER_CC1NP; // both edges generate interrupts

	// set input Pre-Scaler
	TIM5->CCMR1 &= ~TIM_CCMR1_IC1PSC_Msk; // clear bits

	// Enable capture from the counter for channel 1
	TIM5->CCER |= TIM_CCER_CC1E;

	// enable interrupt on channel 1
	TIM5->DIER |= TIM_DIER_CC1IE;

	// enable DMA request for channel 1 (Optional: DMA only)
	TIM5->DIER |= TIM_DIER_CC1DE;

	// enable TIM5 counter
	TIM5->CR1 |= TIM_CR1_CEN;

	// set interrupt priority
	NVIC_SetPriority(TIM5_IRQn, 0); // 0 - highest priority

	// enable TIM5 interrupt on NVIC
	NVIC_EnableIRQ(TIM5_IRQn);

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

	// enable interrupt on channel 1
	TIM5->DIER |= TIM_DIER_CC1IE;

	// enable DMA request for channel 1 (Optional: DMA only)
	TIM5->DIER |= TIM_DIER_CC1DE;

	// Enable capture from the counter for channel 2
	TIM5->CCER |= TIM_CCER_CC2E;

	// enable interrupt on channel 2
	TIM5->DIER |= TIM_DIER_CC2IE;

	// enable DMA request for channel 2 (Optional: DMA only)
	TIM5->DIER |= TIM_DIER_CC2DE;

	// enable TIM5 counter
	TIM5->CR1 |= TIM_CR1_CEN;

	// set interrupt priority
	NVIC_SetPriority(TIM5_IRQn, 0); // highest priority

	// enable TIM5 interrupt on NVIC
	NVIC_EnableIRQ(TIM5_IRQn);

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



