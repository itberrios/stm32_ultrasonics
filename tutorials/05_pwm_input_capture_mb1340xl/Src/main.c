/*
 * Parameters need to be tuned to expected range of PWM inputs :(
 *
 * Are there latencies that need to be accounted for?
 * 			- NVIC latency? --> 12 cycles (clock or bus cycles?)
 * 			- how many clock cycles to propagate through each flip-flop/multiplexer/etc?
 * 			- https://developer.arm.com/documentation/100166/0001/Programmers-Model/Exceptions/Interrupt-latency?lang=en
 * 			- lots of things to consider for about 10us of error, does it really matter? Maybe
 *
 *
 * 	pinouts Nucleo --> MB1304XL:
 * 		5V  --> Vcc
 * 		GND --> GND
 * 		// PA0 --> Pin 4 Tx (echo) (need to hold pin 1 low
 * 		PA0 --> Pin 2 PW (echo)
 * 		PA1 --> Pin 5 RX (trigger) (need 20+us pulse to command range reading)
 *
 *	 OPTIONAL PINOUTS:
 *	    PA8 (test signal) --> trig
 */

#include <stdint.h>
#include "mb1304xl.h"
#include "timer.h"


/* LED on PA5 */
#define LED_PIN       GPIO_ODR_OD5 // 1U << 5
#define PWM_FREQ      1 // 10000 // Hz
#define DUTY_FACTOR   20    // duty factor percentage (0-100)

#define DISTANCE_UNITS    0 // 0 - cm, 1 - in
#define SYS_FREQ          16000000
#define SEC_2_USEC        1000000

/* prototypes */
void init_PA5_LED(void);

/* globals */
const float SYSTEM_CLOCK_FREQUENCY = SYS_FREQ;
const uint32_t INPUT_TIM_PSC = ECHO_TIMER_PSC;

// store variable to scale clocks to seconds
const float CLOCKS_TO_SECONDS = INPUT_TIM_PSC / (float) SYSTEM_CLOCK_FREQUENCY;
const float CLOCKS_TO_US = CLOCKS_TO_SECONDS * SEC_2_USEC;

// get appropriate normalizer to compute measured distances
const float DISTANCE_NORMALIZER = (DISTANCE_UNITS == 0) ? 58.0 : 148.0;



// variables store current Timer counts at detected rising or falling pulse edges
volatile uint32_t count_at_rising = 0;
volatile uint32_t count_at_falling = 0;
volatile uint32_t prev_count_at_rising = 0;
volatile uint32_t overflow_counts = 0;


volatile uint8_t distance_valid = 0;
volatile int32_t pulse_width = 0; // need signed int to account for rollovers
volatile int32_t period = 0;
volatile float pulse_width_us = 0.0;
volatile float period_ms = 0.0;
volatile float distance = 0.0;


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
	config_PA1_AF1();
	config_PWM_TIM2_ch2_trigger();

	// config PA1 echo pin
	config_PA0_AF2();
	// config_TIM5_ch1_echo();
	config_TIM5_ch1_echo_pwm_input();

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



void TIM5_IRQHandler(void)
{
	/* check for timer overflow */
	if ((TIM5->SR & TIM_SR_UIF) != 0)
	{
		// clear UIF flag to prevent re-entry
		TIM5->SR &= ~TIM_SR_UIF;
		overflow_counts++;
	}

	/* check for rising edge interrupt on channel 1*/
	if ((TIM5->SR & TIM_SR_CC1IF) != 0)
	{
		// read CCR1 to clear CC1IF flag
		count_at_rising = TIM5->CCR1;

		// compute period
		period = count_at_rising + ((1 + TIM5->ARR)*overflow_counts);

		// reset overflows since timer restarts on rising edge
		overflow_counts = 0;

	}

	/* check for falling edge interrupt */
	if ((TIM5->SR & TIM_SR_CC2IF) != 0)
	{
		// read CCR2 to clear CC2IF flag
		/* since the counter resets at each rising edge, the falling edge
		 * is equal to the pulse width */
		count_at_falling = TIM5->CCR2;

		// compute Pulse Width in clocks
		// pulse_width = count_at_falling + (1 + TIM5->ARR)*overflow_counts;
		pulse_width = count_at_falling;

		// convert pulse width and period to us
		pulse_width_us = pulse_width * CLOCKS_TO_US;

		// compute distance
		distance = pulse_width_us / DISTANCE_NORMALIZER;

	}

	/* process data */
	// compute Pulse Width and period in clocks
	// pulse_width = count_at_falling + (1 + TIM5->ARR)*overflow_counts;

//	period = (prev_count_at_rising
//			+ ((1 + TIM5->ARR)*overflow_counts))
//			- count_at_rising;

//	// ensure a full period is captured
//	if (overflow_counts < 1)
//	{
//		period += (1 + TIM5->ARR);
//	}

	// convert period to milliseconds
	period_ms = period * CLOCKS_TO_SECONDS * 1000;

	/* do stuff for next callback occurrence if needed */
	// overflow_counts = 0;
	prev_count_at_rising = count_at_rising;

	if (pulse_width <= 10)
	{
		distance_valid = 0;
	}
	else
	{
		distance_valid = 1;
	}

}




