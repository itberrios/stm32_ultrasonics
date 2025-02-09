/*
 *
 *
 * 	pinouts Nucleo --> MB1304XL:
 * 	   (MIGHT) NEED TO SWITCH trigger pin from PA1 to PA0
 * 		5V  --> Vcc
 * 		GND --> GND
 * 		// PA0 --> Pin 4 Tx (echo) (need to hold pin 1 low
 *
 * 		PA0 (A0) --> Pin 5 RX (trigger) (need 20+us pulse to command range reading)
 * 		PA1 (A1) --> Pin 2 PW (Analog Voltage Envelope)
 * 		PA4 (A2) --> Pin 3 AN (Analog Voltage Level)
 * 		PA? (??) --> Pin 5 TX (RS232 msg)
 *
 *	 OPTIONAL PINOUTS:
 *	    PA8 (test signal) --> trig
 *
 *
 *	 NOTE:
 *	 	The ADC normally operates from a 2.4 - 3.6 V supply, so we will only
 *	 	use the 3.3V supply for the sensor. We need to design some type of circuit
 *	 	to use the 5V supply?
 *
 *	 	https://electronics.stackexchange.com/questions/520247/stm32-5v-adc-input
 *
 *	 	This project is in desperate need to filtering, we will use some simple averaging filtering
 *
 *	 	The next project will use the ARM DSP library with ping-pong buffering
 *
 *	 	seems like sensor Ranges 22500us after trigger is applied, might need to use DMA controller to track PWM outputs
 *	 	might be able to use interrupts that collect envelope data from 2250 - 0?
 *	 	or maybe 2500 rolled over to 2000?
 *
 *	 	It doesn't seem like the output pulse is always super high, so we may need to rely on the trigger timing
 */

#include <stdio.h>
#include <stdint.h>
#include "uart.h"
#include "mb1304xl.h"

#define DISTANCE_UNITS    0 // 0 - cm, 1 - in
#define SYS_FREQ          16000000
#define SEC_2_USEC        1000000

#define ENVELOPE_BUFFER_SIZE    100 // hardcode when this is full?
#define LEVEL_BUFFER_SIZE       5   // find a good value for Moving Avg or Rolling Avg

#define TX_DELAY   500 // approx 10us clock cycles for Tx delay from trigger onset


/* globals */
//const float SYSTEM_CLOCK_FREQUENCY = SYS_FREQ;
//const uint32_t INPUT_TIM_PSC = ECHO_TIMER_PSC;
//
//// store variable to scale clocks to seconds
//const float CLOCKS_TO_SECONDS = INPUT_TIM_PSC / (float) SYSTEM_CLOCK_FREQUENCY;
//const float CLOCKS_TO_US = CLOCKS_TO_SECONDS * SEC_2_USEC;



/*
 * NOTE: Something happens after a while (~10min or so) where the measured pulse width
 * will all of a sudden change to a new type of value.
 */

/** globals **/
volatile uint32_t tim2_cnt = 0;
volatile float envelope;
volatile float level;

float envelope_buffer[ENVELOPE_BUFFER_SIZE] = {0.0};
float level_buffer[LEVEL_BUFFER_SIZE] = {0.0};

float avg_envelope;
float avg_level;

uint32_t env_buff_ptr = 0;
uint32_t env_max = 0;
uint32_t env_argmax = 0;


/** prototypes **/
void shift_and_accumulate(float *buffer, uint32_t buffer_size, float value);
float compute_average(float *buffer, uint32_t buffer_size);


int main(void)
{

	// enable FPU for floating point operations
	SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));

	// initialize UART for debug
	uart2_tx_init();

	// debug
	config_adc1_dma();

	// config PA0 as the trigger pin
	config_PA0_AF1();
	config_PWM_TIM2_ch1_trigger();


    /* Loop forever */
	while(1)
	{
		// get current TIM2 counter value
		tim2_cnt = TIM2->CNT;

		/** get data from ADC **/
		// convert to mV
		level = adc_data[0] * ADC_VOLTAGE_SCALE;
		envelope = adc_data[1] * ADC_VOLTAGE_SCALE;

		// convert to cm
		level = adc_data[0] * SAMPLES_TO_CM;
		envelope = adc_data[1] * SAMPLES_TO_CM;

		/** get analog voltage level distance**/
		// shift and acummulate buffers
		shift_and_accumulate(level_buffer, LEVEL_BUFFER_SIZE, level);

		// get moving average
		avg_level = compute_average(level_buffer, LEVEL_BUFFER_SIZE);

		// make it a rolling average
		level_buffer[0] = avg_level;

		/** super easy way, but not efficient **/
		/** Fill analog envelope buffer **/
		if ((tim2_cnt > TX_DELAY)                   // ensure Tx transmission has occured
			& (tim2_cnt <= TRIGGER_TIMER_ARR)       // ensure Counter hasn't reset
			& (env_buff_ptr < ENVELOPE_BUFFER_SIZE) // ensure we don't overstep the buffer memory
			)
		{
			envelope_buffer[env_buff_ptr++] = envelope;
		}

		/** check if buffer is full and get argmax if it is**/
		if (env_buff_ptr >= ENVELOPE_BUFFER_SIZE)
		{
			env_buff_ptr = 0;
			env_argmax = 0;
			env_max = 0;

			for (int i = 0; i < ENVELOPE_BUFFER_SIZE; i++)
			{
				if (envelope_buffer[i] >= env_max)
				{
					env_argmax = i;
					env_max = envelope_buffer[i];
				}
			}
		}

		/** print statements **/
		// print the raw ADC data
		// printf("%d %d\n", adc_data[0], adc_data[1]);

		// print the analog voltages
		// printf("%f %f\n", level, envelope);

		// printf("%f %f\n", level, avg_level);
		// printf("%d %f\n", adc_data[1], envelope);
		// printf("%lu %d %d\n", tim2_cnt, adc_data[0], adc_data[1]);
		printf("%f\n", envelope);


	}
}


/* Shift and accumulate buffer
 * Inputs:
 * 		buffer - buffer array to be shifted
 * 		buffer_size - size of buffer
 * 		value - value to accumulate in position 0
 */
void shift_and_accumulate(float *buffer, uint32_t buffer_size, float value)
{
	// shift buffer by 1 towards the higher index
	for(int i = buffer_size; i > 0; i--)
	{
		buffer[i] = buffer[i - 1];
	}

	// accumulate in position 0
	buffer[0] = value;
}

/** Basic Function to Compuate a Moving Average of an arbitrary array **/
float compute_average(float *buffer, uint32_t buffer_size)
{
	float average = 0.0;
	for (int i = 0; i < buffer_size; i++)
	{
		average += buffer[i];
	}

	return average / (float) buffer_size;
}


