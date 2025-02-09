/*
 * NOTE: This doesn't seem to be able to measure large pulses!?
 *
 */

#include <stdint.h>
#include "hc_sr04.h"
#include "timer.h"


/* LED on PA5 */
#define LED_PIN       GPIO_ODR_OD5 // 1U << 5
#define PWM_FREQ      100 // 10000 // Hz
#define DUTY_FACTOR   20    // duty factor percentage (0-100)

// TEMP: manually enter the input timer PCS in order to scale the received PW
#define INPUT_TIM_PSC    16

#define DISTANCE_UNITS    0 // 0 - cm, 1 - in
#define SYS_FREQ      16000000
#define SEC_2_USEC    1000000

/* prototypes */
void init_PA5_LED(void);

/* globals */
const float SYSTEM_CLOCK_FREQUENCY = SYS_FREQ;

// get appropriate normalizer to compute measured distances
const float DISTANCE_NORMALIZER = (DISTANCE_UNITS == 0) ? 58.0 : 148.0;

volatile int32_t pulse_width = 0; // need to be able to go negative to account for timer rollovers??
volatile uint32_t last_captured = 0;
volatile uint32_t signal_polarity = 0; // assume polarity starts low
volatile uint32_t overflow_counts = 0;

volatile float pulse_width_us = 0.0;
volatile float distance = 0.0;

volatile uint32_t error = 0;


/*
 * NOTE: Something happens after a while (~10min or so) where the measured pulse width
 * will all of a sudden change to a new type of value.
 */


int main(void)
{
	// enable FPU for floating point operations
	SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));

	// TEMP: DEBUG
	// configure test PWM signal on PA8
	config_PA8_AF1();
	config_PWM_TIM1_ch1(PWM_FREQ, DUTY_FACTOR);

	// config PA0 as the trigger pin
	config_PA0_AF1();
	config_PWM_TIM2_ch1_trigger();

	// config PA1 echo pin
	config_PA1_AF2();
	config_TIM5_ch2_echo();

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



/* IRQ Handler for input capture on TIM5 channel 2
 *
 * Actual Pulsee Width ratio can be computed by:
 * 	  PW = pulse_width / (SYS_FREQ / PWM_FREQ)
 * */
void TIM5_IRQHandler(void)
{
	volatile uint32_t current_captured;

	// check if overflow has occurred (need to do this first!)
	if ((TIM5->SR & TIM_SR_UIF) != 0)
	{
		// clear UIF flag to prevent re-entry
		TIM5->SR &= ~TIM_SR_UIF;
		overflow_counts++;
	}

	// check if interrupt flag is set
	if ((TIM5->SR & TIM_SR_CC1IF) != 0)
	{
		// read CCR2 to clear CC2IF flag
        current_captured = TIM5->CCR1;

		// toggle the polarity flag
		signal_polarity = 1 - signal_polarity;

		// update pulse width and distance if measurement is received
		if (signal_polarity == 1)
		{
			pulse_width = current_captured - last_captured;

			// how do we handle rollovers?
			if (pulse_width < 0)
			{
				// pulse_width = pulse_width + (1 + TIM5->ARR)*overflow_counts;
				pulse_width += (1 + TIM5->ARR)*overflow_counts;
			}

			// convert pulse width from clocks to micro seconds
			pulse_width_us = pulse_width; //  * CLOCKS_TO_US;

			// compute distance
			distance = pulse_width_us / DISTANCE_NORMALIZER;

		} // end if

		// store most recent clock counts and clear overflow counts
		last_captured = current_captured;
		overflow_counts = 0;

	} // end if

}



/* IRQ Handler for input capture on TIM5 channel 2
 *
 * Actual Pulsee Width ratio can be computed by:
 * 	  PW = pulse_width / (SYS_FREQ / PWM_FREQ)
 * */
void TIM5_IRQHandler_OLD(void)
{
	// check if overflow has occurred (need to do this first!)
	if ((TIM5->SR & TIM_SR_UIF) != 0)
	{
		// clear UIF flag to prevent re-entry
		TIM5->SR &= ~TIM_SR_UIF;
		overflow_counts++;
	}

	volatile uint32_t current_captured;

	// check if interrupt flag is set
	if ((TIM5->SR & TIM_SR_CC2IF) != 0)
	{
		// read CCR2 to clear CC2IF flag
        current_captured = TIM5->CCR2;

		// toggle the polarity flag
		signal_polarity = 1 - signal_polarity;

		// update pulse width and distance if measurement is received
		if (signal_polarity == 1)
		{
			pulse_width = (current_captured - last_captured);
			// pulse_width = (current_captured - last_captured) + ((1 + TIM5->ARR) * overflow_counts);
			// overflow_counts = 0;

			// how do we handle rollovers?
			if (pulse_width < 0)
			{
				// pulse_width = pulse_width + (1 + TIM5->ARR); //  * overflow_counts;
				pulse_width = pulse_width + (1 + TIM5->ARR) * overflow_counts;
				// overflow_counts = 0;
			}

			if ((pulse_width != 400) & (pulse_width != 1200))
			{
				error++;
			}

			// convert pulse width from clocks to micro seconds
			pulse_width_us = pulse_width
					         * (INPUT_TIM_PSC / (float) SYSTEM_CLOCK_FREQUENCY)
							 * SEC_2_USEC;

			// compute distance
			distance = pulse_width_us / DISTANCE_NORMALIZER;

		} // end if

		// store counts
		last_captured = current_captured;
		overflow_counts = 0;

	} // end if



	/* This doesn't seem to do anything and it seems to cause
	 * stability issues in the pulse_width measurement?? i.e. it flips
	 * between active low and active high
	 */
//	// check if Overcapture has occured
//	if ((TIM5->SR & TIM_SR_CC2OF) != 0)
//	{
//		// clear channel 2 overcapture flag
//		TIM5->SR &= ~TIM_SR_CC2OF;
//	}

}


