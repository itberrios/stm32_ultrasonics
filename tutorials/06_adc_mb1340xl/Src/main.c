/*
 *
 *
 * 	pinouts Nucleo --> MB1304XL:
 * 	   (MIGHT) NEED TO SWITCH trigger pin from PA1 to PA0
 * 		5V  --> Vcc
 * 		GND --> GND
 * 		// PA0 --> Pin 4 Tx (echo) (need to hold pin 1 low
 *
 * 		PA0 --> Pin 5 RX (trigger) (need 20+us pulse to command range reading)
 * 		PA1 --> Pin 2 PW (Analog Voltage Envelope)
 * 		PA4 --> Pin 3 AN (Analog Voltage Level)
 * 		PA?? --> Pin 5 TX (RS232 msg)
 *
 *	 OPTIONAL PINOUTS:
 *	    PA8 (test signal) --> trig
 */

#include <stdio.h>
#include <stdint.h>
#include "uart.h"
#include "mb1304xl.h"

#define DISTANCE_UNITS    0 // 0 - cm, 1 - in
#define SYS_FREQ          16000000
#define SEC_2_USEC        1000000

/* globals */
//const float SYSTEM_CLOCK_FREQUENCY = SYS_FREQ;
//const uint32_t INPUT_TIM_PSC = ECHO_TIMER_PSC;
//
//// store variable to scale clocks to seconds
//const float CLOCKS_TO_SECONDS = INPUT_TIM_PSC / (float) SYSTEM_CLOCK_FREQUENCY;
//const float CLOCKS_TO_US = CLOCKS_TO_SECONDS * SEC_2_USEC;




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

	// initialize UART for debug
	uart2_tx_init();

	// debug
	config_adc1_dma();

	// config PA0 as the trigger pin
//	config_PA0_AF1();
//	config_PWM_TIM2_ch1_trigger();

	// configure ADC
//	config_PA1_ADC1_1();
//	config_adc1_ch1();


    /* Loop forever */
	while(1)
	{
		printf("%d %d\n", adc_data[0], adc_data[1]);

	}
}





