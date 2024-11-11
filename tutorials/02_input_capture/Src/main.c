/*
 * TODO: redo this whole project with code that is clean and concise
 */


#include <stdint.h>
#include "timer.h"
#include "input_timer.h"


/* LED on PA5 */
#define LED_PIN       GPIO_ODR_OD5 // 1U << 5
#define PWM_FREQ      10000 // Hz
#define DUTY_FACTOR   25 // duty factor percentage (0-100)

#define SYS_FREQ    16000000

/* prototypes */
void init_PA5_LED(void);

/* globals */
volatile int32_t _temp;
volatile int32_t temp;
volatile uint32_t pw_error_1; // unsigned int errors
volatile uint32_t pw_error_2; // signed int errors
volatile uint32_t pw_error_3;
volatile uint32_t pw_error_4;
volatile uint32_t pulse_width = 0; // need to be able to go negative to account for timer rollovers
volatile uint32_t last_captured = 0;
volatile uint32_t signal_polarity = 0; // assume polarity starts low
volatile uint32_t overflow_counts = 0;
volatile uint32_t total_overflow_counts = 0;

volatile float pw_seconds;

/*
 * NOTE: Something happens after a while (~10min or so) where the measured pulse width
 * will all of a sudden change to a new type of value.
 */


int main(void)
{
	// enable FPU for floating point operations
	SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));

	// initalize PA8 to AF1 for TIM1
	config_PA8_AF1();

	// configure TIM1 channel 1 for PWM
	init_PWM_TIM1_ch1(PWM_FREQ, DUTY_FACTOR);

	// configure Input Compare on PA15
	config_PA15_TIM2_1_input();

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
		total_overflow_counts++;
		TIM3->SR &= ~TIM_SR_UIF; // clear event flag
	}
}


/* IRQ Handler for input capture on TIM2 channel 1
 *
 * Actual Pulsee Width ratio can be computed by:
 * 	  PW = pulse_width / (SYS_FREQ / PWM_FREQ)
 * */
void TIM2_IRQHandler(void)
{
	volatile uint32_t current_captured;

	// check if overflow has occurred
	if ((TIM2->SR & TIM_SR_UIF) != 0)
	{
		// clear UIF flag to prevent re-entry
		TIM2->SR &= ~TIM_SR_UIF;
		overflow_counts++;
		total_overflow_counts++;

		// NOTE: last/current captured goes to 3 around this time
	}

	// check if interrupt flag is set
	if ((TIM2->SR & TIM_SR_CC1IF) != 0)
	{
		// read CCR1 to clear CC1IF flag
		/** NOTE: only the upper 16 bits from TIM2->CNT get latched to TIM2->CCR1 **/
        current_captured = TIM2->CCR1;
		// current_captured = (TIM2->CCR1 & 0xFFFF0000) >> 16; // try using only upper 16 bits??

		// toggle the polarity flag
		signal_polarity = 1 - signal_polarity;

		if (signal_polarity == 1)
		{
			pulse_width = current_captured - last_captured;
//			pulse_width = (current_captured - last_captured)
//					      + (1 + TIM2->ARR)*overflow_counts;
			_temp = current_captured - last_captured;
//			temp = (current_captured - last_captured)
//					+ (1 + TIM2->ARR)*overflow_counts;

			// account for rollovers an overflows (seems to only work with 0xFFFF ARR??
			if (_temp < 0)
			{
				// temp = _temp + (1 + TIM2->ARR);
				temp = _temp + (1 + TIM2->ARR)*overflow_counts;
			}
			else
			{
				temp = _temp;
			}

//			if (temp < 0)
//			{
//				if (overflow_counts <= 1)
//				{
//					temp = temp + (1 + TIM2->ARR);
//				}
//				else
//				{
//					temp = temp + (1 + TIM2->ARR)*overflow_counts;
//				}
//			}

			// TEMP: DEBUG
			if (pulse_width > 1500)
			{
				pw_error_1++;
			}
			if (temp > 1500)
			{
				pw_error_2++;
			}
			else if (temp < 0)
			{
				pw_error_3++;
			}

			if ((temp != 400) & (temp != 1200))
			{
				pw_error_4++;
			}


			overflow_counts = 0;
//			if (pulse_width < 0)
//			{
//				pulse_width = pulse_width + TIM2->ARR;
//			}


			// compute pulse width in seconds
			pw_seconds = pulse_width / (float) SYS_FREQ;

		} // end if

		// store counts
		last_captured = current_captured;

	} // end if



	/* This doesn't seem to do anything and it seems to cause
	 * stability issues in the pulse_width measurement?? i.e. it flips
	 * between active low and active high
	 */
//	// check if Overcapture has occured
//	if ((TIM2->SR & TIM_SR_CC1OF) != 0)
//	{
//		// clear channel 1 overcapture flag
//		TIM2->SR &= ~TIM_SR_CC1OF;
//	}

}




