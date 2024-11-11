#include "input_timer.h"




/* Setup PA15 for input compare on TIM2 channel 1 */
void config_PA15_TIM2_1_input(void)
{
	/** config PA15 for input compare **/
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// set PA15 to Alternate Function mode
	GPIOA->MODER &= ~GPIO_MODER_MODE15_0;
	GPIOA->MODER |= GPIO_MODER_MODE15_1;

	// set PA15 to AF1
	GPIOA->AFR[1] &= ~GPIO_AFRH_AFRH7;  // clear bits
	GPIOA->AFR[1] |= GPIO_AFRH_AFRH7_0; // 0001 for AF1

	/** config TIM2, channel 1 for input compare **/
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	// set Pre-Scaler
	// TIM2->PSC = 160 - 1; // slow clock to clock to 100 kHz --> VALIF CONFIG
	TIM2->PSC = 0; // test different pre-scaler values

	// set ARR to max value
	// TIM2->ARR = 0xFFFFFFFF; // --> VALID CONFIG
	TIM2->ARR = 0xFFFF; // 59999; // 0x0000FF00; // 0xFFFFFFFF;

	// select channel 1 as the active input
	TIM2->CCMR1 &= ~TIM_CCMR1_CC1S_Msk; // clear bits
	TIM2->CCMR1 |= TIM_CCMR1_CC1S_0;    // 01 - maps ch1 to TIM1

	// setup input capture filtering
	TIM2->CCMR1 &= ~TIM_CCMR1_IC1F_Msk;

	// select the edge of transition
	TIM2->CCER |= TIM_CCER_CC1P | TIM_CCER_CC1NP; // both edges generate interrupts

	// set input Pre-Scaler
	TIM2->CCMR1 &= ~TIM_CCMR1_IC1PSC_Msk; // clear bits

	// Enable capture from the counter for channel 1
	TIM2->CCER |= TIM_CCER_CC1E;

	// enable interrupt on channel 1
	TIM2->DIER |= TIM_DIER_CC1IE;

	// enable DMA request for channel 1 (Optional: DMA only)
	TIM2->DIER |= TIM_DIER_CC1DE;

	// enable TIM2 counter
	TIM2->CR1 |= TIM_CR1_CEN;

	// set interrupt priority
	NVIC_SetPriority(TIM2_IRQn, 0); // highest priority

	// enable TIM2 interrupt on NVIC
	NVIC_EnableIRQ(TIM2_IRQn);

}
