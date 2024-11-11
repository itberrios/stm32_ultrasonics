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
	// initialize LED timer
	// config_LED_TIM2();

	// initialize LED
	init_PA5_LED();


	// initialize timer on PA8 (D7 on the nucleo board)
	// config_PA8_ch1_TIM1();

	// initialize update counter to counter every ## milliseconds
	update_counter(5);

    /* Loop forever */
	while(1)
	{
		 delay(10000);

		 // toggle LED
		 GPIOA->ODR ^= LED_PIN;
	}
}


/**
 * Standalonf function to initialize the LED on PA5
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


