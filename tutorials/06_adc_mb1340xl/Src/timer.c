/*
 * led_timer.c
 *
 *  Created on: Oct 1, 2024
 *      Author: iberrios
 *
 *  Program to setup and toggle the built-in LED on PA5 with a General Purpose Timer
 */

#include "timer.h"



/** DEFINES **/
#define SYS_FREQ    16000000

#define TIM2_PSC    8000 - 1 // Prescaler to slow clock to 2kHz
#define TIM2_ARR    2000 - 1 // scale the prescaled clock to desired timer rate of 1 Hz

#define TIM1_PSC    8000 - 1 // Prescaler to slow clock to 2kHz
#define TIM1_ARR    250 - 1  // scale the prescaled clock to desired timer rate 8Hz

#define DELAY_PSC   16000 - 1 // slows clock to 1 kHz


/** PROTOYPES **/
//void config_PA5_AF1(void);
//void config_PA8_AF1(void);


/* configures TIM2 channel for built in F411RE nucleo LED on PA5
 */
void config_LED_TIM2(void)
{
	/** configure LED on PA5 **/
	config_PA5_AF1();

	/** configure TIM2 **/
	// enable TIM2
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	// set counting direction to up
	TIM2->CR1 &= ~TIM_CR1_DIR;

	// set clock prescaler to slow clock down to 2kHz
	TIM2->PSC = TIM2_PSC;

	// set auto reload register to account for 2kHz clock and set timer to 1Hz
	TIM2->ARR = TIM2_ARR;

	// set capture compare register, this enables the LED to toggle?
	// should be any value between 0 - ARR
	TIM2->CCR1 = 500;

	// select the toggle mode for channel 1
	TIM2->CCMR1 &= ~TIM_CCMR1_OC1M_Msk; // clear bits
	TIM2->CCMR1 |= (TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1); // set to 011

	// enable preload of register CCR1 (update synchronously)
	TIM2->CCMR1 |= TIM_CCMR1_OC1PE;

	// select output polarity
	TIM2->CCER |= TIM_CCER_CC1P; // 0 = active high

	// enable output of channel 1
	TIM2->CCER |= TIM_CCER_CC1E;

	// enable channels in BDTR?? seems like this is only for Advanced Timers

	// reset counter to 0
	TIM2->CNT = 0;

	// start timer counter
	TIM2->CR1 |= TIM_CR1_CEN;

}


/*
 * Configure PA8 for channel1 of TIM1 (Advanced Timer on PA7)
 */
void config_PA8_ch1_TIM1(void)
{
	/** configure PA8 for Timer usage**/
	config_PA8_AF1();

	/** configure TIM1 **/
	// enable TIM1
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

	// set counting direction to up
	TIM1->CR1 &= ~TIM_CR1_DIR;

	// set clock prescaler to slow clock down to 2kHz
	TIM1->PSC = TIM1_PSC;

	// set auto reload register to account for 2kHz clock and set timer to 1Hz
	TIM1->ARR = TIM1_ARR;

	// set capture compare register, this enables the LED to toggle?
	// should be any value between 0 - ARR
	TIM1->CCR1 = 100;

	// select the toggle mode for channel 1
	TIM1->CCMR1 &= ~TIM_CCMR1_OC1M_Msk; // clear bits
	TIM1->CCMR1 |= (TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1); // set to 011

	// enable preload of register CCR1 (update synchronously)
	TIM1->CCMR1 |= TIM_CCMR1_OC1PE;

	// select output polarity
	TIM1->CCER |= TIM_CCER_CC1P; // 0 = active high, 1 = active low

	// enable output of channel 1
	TIM1->CCER |= TIM_CCER_CC1E;

	// Main Output Enable (MOE), globally enables all channels
	TIM1->BDTR |= TIM_BDTR_MOE;

	// reset counter to 0
	// TIM1->CNT = 0;

	// start timer counter
	TIM1->CR1 |= TIM_CR1_CEN;

}

/** Delay by set number of ms **/
void delay(uint16_t ms)
{
	if (ms == 0) return;

	// enable clock access to TIM4
	RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

	// ensure TIM4 is turned off prior to config
	TIM4->CR1 &= ~TIM_CR1_CEN;

	// clear status register and counter
	TIM4->SR = 0;
	TIM4->CNT = 0;

	// set prescaler
	TIM4->PSC = DELAY_PSC;

	// set desired Auto Reload (ARR) value for time delay in ms
	TIM4->ARR = ms - 1;

	// RCR??

	// enable TIM4
	TIM4->CR1 |= TIM_CR1_CEN;

	// delay
	while( (TIM4->SR & TIM_SR_UIF) == 0 ) {}

}


/**
 * Counts the number of update events
 * i.e. counts the number of increments
 *     if ms = 1000, the external update handler in
 *     TIM3_IRQHandler() will count the number of seconds
 */
void update_counter(uint16_t ms)
{
	// enable clock access to TIM3
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

	// ensure TIM3 is turned off prior to config
	TIM3->CR1 &= ~TIM_CR1_CEN;

	// clear status register and counter
	TIM3->SR = 0;
	TIM3->CNT = 0;

	// set counting direction to up
	TIM3->CR1 &= ~TIM_CR1_DIR;

	// set prescaler
	TIM3->PSC = DELAY_PSC;

	// set desired Auto Reload (ARR) value for time delay in ms
	TIM3->ARR = ms - 1;

	// enable DMA interrupts
	TIM3->DIER |= TIM_DIER_UIE;

	// enable timer interrupt in NVIC
	NVIC_EnableIRQ(TIM3_IRQn);

	// enable TIM3
	TIM3->CR1 |= TIM_CR1_CEN;

	// delay
	while( (TIM3->SR & TIM_SR_UIF) == 0 ) {}

}


/* Setup PWM timer on TIM1 channel 1 for LED dimming
 */
void init_PWM_LED_control(void)
{
	// enable clock access to TIM1
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

	// ensure TIM1 is disabled
	// TIM1->CR1 &= ~TIM_CR1_CEN;

	// set counting direction
	TIM1->CR1 &= ~TIM_CR1_DIR;

	// set prescaler
	TIM1->PSC = 160 - 1; // set clock to 100kHz

	// set Auto Reload Value
	TIM1->ARR = 1000 - 1; // ??

	// set output compare register for channel 1
	TIM1->CCR1 = 500; // sets duty cycle

	// select PWM mode for channel 1
	TIM1->CCMR1 &= ~TIM_CCMR1_OC1M_Msk; // clear bits
	TIM1->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2); // 110 - PWM mode 1

	// enable register preload
	TIM1->CCMR1 |= TIM_CCMR1_OC1PE;

	// slect output polarity: 0 - active high, 1 - active low
	TIM1->CCER &= ~TIM_CCER_CC1P;

	// enable output of channel 1
	TIM1->CCER |= TIM_CCER_CC1E;

	// enable all outputs
	TIM1->BDTR |= TIM_BDTR_MOE;

	// enable TIM1
	TIM1->CR1 |= TIM_CR1_CEN;

}


/* Setup PWM timer on TIM1 channel 1 for LED dimming
 *
 * inputs:
 * 	   pwm_freq - PWM frequency in Hertz (max: 1.6MHz)
 * 	   duty - duty factor percentage 0-100
 *
 * 	   NOTE: Rounding errors in actual PWM Freqeuncy and Duty factor
 * 	       might occur due to use of unisgned integers
 */
void config_PWM_TIM1_ch1(uint32_t pwm_freq, uint16_t duty)
{
	// sanity checks
	if (pwm_freq > 1600000) pwm_freq = 1600000;
	if (duty > 100) duty = 100;

	// enable clock access to TIM1
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

	// ensure TIM1 is disabled
	TIM1->CR1 &= ~TIM_CR1_CEN;

	// set counting direction
	TIM1->CR1 &= ~TIM_CR1_DIR;

	// set prescaler
	// TIM1->PSC = 0; // don't slow the clock, keep at 16MHz
	TIM1->PSC = 160; // slow the clock down

	// compute ARR value and compare value for duty cycle
	uint16_t arr = SYS_FREQ / (pwm_freq);
	uint16_t ccr1 = (duty * 0.01) * arr;

	// set Auto Reload Value
	TIM1->ARR = arr - 1;

	// set output compare register for channel 1
	TIM1->CCR1 = ccr1; // sets duty cycle

	// select PWM mode for channel 1
	TIM1->CCMR1 &= ~TIM_CCMR1_OC1M_Msk; // clear bits
	TIM1->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2); // 110 - PWM mode 1

	// enable register preload
	TIM1->CCMR1 |= TIM_CCMR1_OC1PE;

	// slect output polarity: 0 - active high, 1 - active low
	TIM1->CCER &= ~TIM_CCER_CC1P;

	// enable output of channel 1
	TIM1->CCER |= TIM_CCER_CC1E;

	// enable all outputs
	TIM1->BDTR |= TIM_BDTR_MOE;

	// enable TIM1
	TIM1->CR1 |= TIM_CR1_CEN;

}


/** GPIO Pin functions **/

/** configure LED on PA5 for TIMER usage**/
void config_PA5_AF1(void)
{
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// enable Alternate Function mode for PA5
	GPIOA->MODER &= ~GPIO_MODER_MODE5_Msk; // clear Mode bits
	GPIOA->MODER |= GPIO_MODER_MODE5_1;     // set mode bits to 10 for AF

	// set PA5 to AF1 for TIM2
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL5_Msk; // clear AFRL bits
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL5_0;    // set AFRL5 to 0001

	// set Output Speed to low
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR5;

	// ensure PA5 has no pull-up or pull-down
	GPIOA->PUPDR &= GPIO_PUPDR_PUPD5_Msk;
}

/** configure PA8 for Timer usage**/
void config_PA8_AF1(void)
{
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// enable Alternate Function mode for PA8
	GPIOA->MODER &= ~GPIO_MODER_MODE8_Msk; // clear Mode bits
	GPIOA->MODER |= GPIO_MODER_MODE8_1;    // set mode bits to 10 for AF

	// set PA8 to AF1 for TIM1
	GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL8_Msk; // clear AFRL bits
	GPIOA->AFR[1] |= GPIO_AFRH_AFRH0_0;     // set AFRL8 to 0001 for AF1

	// set Output Speed to low
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR8;

	// ensure PA8 has no pull-up or pull-down
	GPIOA->PUPDR &= GPIO_PUPDR_PUPD8_Msk;
}
