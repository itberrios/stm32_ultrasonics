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
 *	 	use the 3.3V supply for the sensor.
 */

#include <stdio.h>
#include <stdint.h>
#include "arm_math.h"
#include "uart.h"
#include "mb1304xl.h"

#define DISTANCE_UNITS    0 // 0 - cm, 1 - in
#define SYS_FREQ          16000000
#define SEC_2_USEC        1000000

#define AVG_BUFFER_SIZE   7   // find a good value for Moving Avg or Rolling Avg

/** globals **/
const float CLOCKS_2_SEC = SONAR_TRIGGER_PSC / (float) SYS_FREQ;
const float CLOCKS_2_US =  (SONAR_TRIGGER_PSC / (float) SYS_FREQ) * 1000000;
volatile float envelope;
volatile float level;
volatile float distance;
volatile float average_distance;

float averaging_buffer[AVG_BUFFER_SIZE] = {0.0};
float rolling_average;


/** prototypes **/
void shift_and_accumulate(float *buffer, uint32_t buffer_size, float value);
float compute_average(float *buffer, uint32_t buffer_size);


int main(void)
{

	// enable FPU for floating point operations
	SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));

	// initialize UART for debug
	uart2_tx_init();

	// configure sonar trigger (Optional)
	config_PA0_AF2();
	config_PWM_TIM5_ch1_trigger();

	// configure ADC with DMA for 2 inputs into PA1 and PA4
	config_adc1_dma();

    /* Loop forever */
	while(1)
	{


	}
}

/** DMA interrupt request handler
 * maybe instead of stopping the adc we just stop the adc conversion timer
 * **/
void DMA2_Stream0_IRQHandler(void)
{
	// check if DMA transfer has completed
	if ((DMA2->LISR & DMA_LISR_TCIF0) == DMA_LISR_TCIF0)
	{
		/** Do something with the data **/
		// print raw values
		average_distance = 0.0; // reset
		for (int i = 0; i < ADC_BUFFER_SIZE; i += 2)
		{

			// get measured distance
			distance = adc_data_buffer[i] * SAMPLES_TO_CM;
			average_distance += distance;

			// print the raw data!
            // printf("%lu %d %d \n", TIM5->CNT, adc_data_buffer[i], adc_data_buffer[i+1]);

			// print snapshot (allows us to see how noisy the raw data really is)
			printf("%f %f %f \n", distance, average_distance / (float) ((i + 2)/2), rolling_average);
		}

		// get average from DMA buffer
		average_distance = average_distance / (float) NUM_CONVERSIONS;

		/** get analog voltage level distance**/
		// shift and acummulate buffers
		shift_and_accumulate(averaging_buffer, AVG_BUFFER_SIZE, average_distance);

		// get moving average
		rolling_average = compute_average(averaging_buffer, AVG_BUFFER_SIZE);

		// make it a rolling average (COMMENT THIS OUT AND NOTICE THE DIFFERENCE!)
		// averaging_buffer[0] = rolling_average;


		printf("%f %f %f \n", distance, average_distance, rolling_average);


		// (OPTIONAL) force zeros into trace to denote sampling periods
//		for (int i = 0; i < 3; i++)
//		{
//			printf("%d %d %d \n", 0, 0, 0);
//		}

	}

	// clear flags
	DMA2->LIFCR |= (DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0);
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
