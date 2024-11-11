#include <stdint.h>
#include "timer.h"


/* LED on PA5 */
#define LED_PIN     GPIO_ODR_OD5 // 1U << 5


/* prototypes */
void init_PA5_LED(void);

/* globals */
volatile uint32_t counts;


int main(void)
{
	// initalize PA8 to AF1 for TIM1
	config_PA8_AF1();

	// configure TIM1 channel 1 for PWM
	init_PWM_TIM1_ch1(1000, 50);

	// TEMP
	// config_PA8_ch1_TIM1();

    /* Loop forever */
	while(1)
	{

	}
}


/**
 * Standalone function to initialize the LED on PA5 to output mode
 */
void init_PA5_LED(void)
{

	/** configure LED on PA5 **/
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// set to general output mode
	GPIOA->MODER |= GPIO_MODER_MODE5_0;
	GPIOA->MODER &= ~GPIO_MODER_MODE5_1;
}


/*
 * IRQ handler for TIM3 update events
 */
void TIM3_IRQHandler(void)
{
	if ((TIM3->SR & TIM_SR_UIF) != 0)
	{
		counts++;
		TIM3->SR &= ~TIM_SR_UIF; // clear event flag
	}
}


