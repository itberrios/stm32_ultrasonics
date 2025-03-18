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


/** prototypes **/
float compute_adc_sample_period(void);
float compute_range_gate(float);

/** globals **/
//float ADC_CLOCK_FREQ = SYS_CLOCK_FREQ / (float) ADC_TIMER_PSC;
//const float ADC_SAMPLE_PERIOD = ((float) ADC_TIMER_ARR);

#define SYS_CLOCK_FREQ_2 16000000

const float CLOCKS_2_SEC = SONAR_TRIGGER_PSC / (float) SYS_CLOCK_FREQ_2;
const float CLOCKS_2_US =  (SONAR_TRIGGER_PSC / (float) SYS_CLOCK_FREQ_2) * 1000000;

// redefine the macros as constant variables here
const int _SYS_CLOCK_FREQ = SYS_CLOCK_FREQ;
const int _ADC_TIMER_PSC = ADC_TIMER_PSC;
const int _ADC_TIMER_ARR = ADC_TIMER_ARR;
const float _SOUND_METERS_PER_SEC = SOUND_METERS_PER_SEC;


// number of bins that comprise the transmit pulse
#define NUM_XMIT_BINS    74


/** control the ADC DMA Double Buffering **/
uint16_t *adc_data_ptr; // initialize pointer to current buffer (ping or pong)
volatile uint8_t ADC_DMA_COMPLETE = 0; // 'semaphore' to handle program flow control

/** data processing variables **/
// volatile float32_t analog_envelope[NUM_CONVERSIONS]; // sampled analog voltage envelope
// volatile float32_t sensor_range[NUM_CONVERSIONS];    // range as measurted by the sensor

volatile float32_t envelope_range;
volatile float32_t sensor_range;
volatile uint16_t peak_index = 0;
uint16_t idx = 0;



int main(void)
{

	// enable FPU for floating point operations
	SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));

	// compute values once FPU is active
//	const float ADC_SAMPLE_PERIOD = compute_adc_sample_period();
//	const float RANGE_GATE = compute_range_gate(ADC_SAMPLE_PERIOD); // 0.006 m

	const float ADC_CLOCK_FREQ = SYS_CLOCK_FREQ / (float) ADC_TIMER_PSC;
	const float ADC_SAMPLE_PERIOD = ADC_TIMER_ARR / ADC_CLOCK_FREQ;
	// Range in meters for each ADC sample
	const float RANGE_GATE = (SOUND_METERS_PER_SEC * ADC_SAMPLE_PERIOD)/2.0;

	// initialize UART for debug
	uart2_tx_init();

	// configure ADC conversion trigger
	config_TIM2_ch4_adc_trigger();

	// config PA0 as the sensor trigger pin
	config_PA0_AF2();
	config_PWM_TIM5_ch1_trigger();

	// configure ADC sampling trigger
	config_PWM_TIM3_ch1_trigger();

	// configure ADC with DMA for 2 inputs into PA1 and PA4
	config_adc1_dma();

    /* Loop forever */
	while(1)
	{
		/** process ADC data **/
		if (ADC_DMA_COMPLETE)
		{

			// perform arg max straight from buffer
			peak_index = NUM_CONVERSIONS - 1;
			for (uint16_t i = 1; i < ADC_BUFFER_SIZE; i += 2)
			{
				// print raw voltage to trace
				printf("%d \n", adc_data_ptr[i]);

				// find arg max
				if (i > NUM_XMIT_BINS) // throw out samples that occur during xmit
				{
					peak_index = (adc_data_ptr[i] > adc_data_ptr[peak_index]) ? i : peak_index;
				}
			}

			// printf("%d \n", 0);

			// get sensor range for comparison
			sensor_range = adc_data_ptr[0]*SAMPLES_TO_CM;

			// compute envelope range
			// (divide peak index by 2 to account for interleaved indexing in 'adc_data_ptr')
			envelope_range = (float32_t) peak_index / 2.0 * RANGE_GATE * 100.0;

			// display voltages
			// printf("%f %f \n", sensor_range, envelope_range);

			// reset ADC DMA semaphore once we're done
			ADC_DMA_COMPLETE = 0;
		}
	}
}

/** DMA interrupt request handler
 * maybe instead of staoping the adc we just stop the adc conversion timer??
 * **/
void DMA2_Stream0_IRQHandler(void)
{
	// check if DMA transfer has completed
	if ((DMA2->LISR & DMA_LISR_TCIF0) == DMA_LISR_TCIF0)
	{
		// turn off TIM 2 ADC trigger
		TIM2->CR1 &= ~TIM_CR1_CEN;

		// set pointer to current data buffer
		if ((DMA2_Stream0->CR & DMA_SxCR_CT) == DMA_SxCR_CT)
		{
			// CT = 1 and Memory 1 is the target
			adc_data_ptr = adc_data_buffer_pong;
		}
		else
		{
			// CT = 1 and Memory 0 is the target
			adc_data_ptr = adc_data_buffer_ping;
		}
	}

	// clear flags
	DMA2->LIFCR |= (DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0);

	// update semaphore
	ADC_DMA_COMPLETE = 1;
}



/* IRQ Handler for TIM3 channel 1
 * Can't seem to use NVIC interrupts for TIM2 ch1 and TIM5 ch1, why?
 *
 * This IRQ Handler starts the ADC sampling by enabling TIM2 which
 * is used as an 'external' trigger for the ADC. The ADC takes a sample
 * at each
 * */
void TIM3_IRQHandler(void)
{
	// check for Capture Compare 1 interrupt
	if ((TIM3->SR & TIM_SR_CC1IF) != 0)
	{
		// clear update interrupt flag
		TIM3->SR &= ~TIM_SR_CC1IF;


		// printf("%lu %d %d \n", TIM5->CNT, 20, 20);

		// enable TIM 2 ADC trigger
		TIM2->CR1 |= TIM_CR1_CEN;
	}
}

/** functions to call once the FPU is active **/

float compute_adc_sample_period(void)
{
	float ADC_CLOCK_FREQ = SYS_CLOCK_FREQ / (float) ADC_TIMER_PSC;
	float ADC_SAMPLE_PERIOD = ADC_TIMER_ARR / ADC_CLOCK_FREQ;

	return ADC_SAMPLE_PERIOD;
}

float compute_range_gate(float ADC_SAMPLE_PERIOD)
{
	// Range in meters for each ADC sample
	float RANGE_GATE = (SOUND_METERS_PER_SEC * ADC_SAMPLE_PERIOD)/2;

	return RANGE_GATE;
}
