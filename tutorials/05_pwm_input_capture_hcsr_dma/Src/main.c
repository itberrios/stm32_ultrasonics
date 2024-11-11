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
 * 	pinouts Nucleo --> HC-SR04:
 * 		5V  --> Vcc
 * 		GND --> GND
 * 		PA0 --> Echo
 * 		PA1 --> Trig
 *
 *	 OPTIONAL PINOUTS:
 *	    PA8 (test signal) --> trig
 */

#include <stdint.h>
#include "hc_sr04.h"

#define DISTANCE_UNITS    0 // 0 - cm, 1 - in
#define SYS_FREQ          16000000
#define SEC_2_USEC        1000000


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
volatile uint32_t overflow_counts = 0;

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


	// config PA1 as the trigger pin
	config_PA1_AF1();
	config_PWM_TIM2_ch2_trigger();

	// config PA0 echo pin
	config_PA0_AF2();
	config_TIM5_ch1_echo_pwm_input();

    /* Loop forever */
	while(1)
	{

	}
}



/** Rising Edge Stream Handler **/
void DMA1_Stream2_IRQHandler(void)
{
	/* check for timer overflow */
	if ((TIM5->SR & TIM_SR_UIF) != 0)
	{
		// clear UIF flag to prevent re-entry
		TIM5->SR &= ~TIM_SR_UIF;
		overflow_counts++;
	}

	// check if DMA transfer has completed
	if ((DMA1->LISR & DMA_LISR_TCIF2) == DMA_LISR_TCIF2)
	{
		// get clock counts at rising edge (will be greater than 0 due to rise time)
		count_at_rising = tim5_channel1_ccr[0];

		// compute period
		period = count_at_rising + ((1 + TIM5->ARR)*overflow_counts);

		// convert period to milliseconds
		period_ms = period * CLOCKS_TO_SECONDS * 1000;

		// reset overflows since timer restarts on rising edge
		overflow_counts = 0;
	}

	// clear flags
	DMA1->LIFCR |= (DMA_LIFCR_CTCIF2 | DMA_LIFCR_CHTIF2 | DMA_LIFCR_CTEIF2);
}


/** Falling Edge Stream Handler **/
void DMA1_Stream4_IRQHandler(void)
{

	if ((DMA1->HISR & DMA_HISR_TCIF4) == DMA_HISR_TCIF4)
	{
		// get pulse width in clocks
		pulse_width = tim5_channel2_ccr[0];

		// convert pulse width to microseconds
		pulse_width_us = pulse_width * CLOCKS_TO_US;

		// compute distance
		distance = pulse_width_us / DISTANCE_NORMALIZER;
	}

	// clear flags
	DMA1->HIFCR |= (DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4);
}





