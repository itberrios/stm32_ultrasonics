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
 *
 *	 	The next project will use the ARM DSP library with ping-pong buffering
 */

#include <stdio.h>
#include <stdint.h>
#include "arm_math.h"
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


/** globals **/
const float CLOCKS_2_SEC = SONAR_TRIGGER_PSC / (float) SYS_FREQ;
const float CLOCKS_2_US =  (SONAR_TRIGGER_PSC / (float) SYS_FREQ) * 1000000;
volatile float analog_envelope;
volatile float analog_level;
volatile uint32_t timer_cnt;
volatile float tim2_timer;


int main(void)
{

	// enable FPU for floating point operations
	SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));

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

		/** Do something with the data **/
		// print raw values
		for (int i = 0; i < ADC_BUFFER_SIZE; i += 2)
		{
			printf("%lu %d %d \n", TIM5->CNT, adc_data_buffer[i], adc_data_buffer[i+1]);
		}

		for (int i = 0; i < 3; i++)
		{
			printf("%d %d %d \n", 0, 0, 0);
		}

	}

	// clear flags
	DMA2->LIFCR |= (DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0);
}



/* IRQ Handler for TIM3 channel 1
 * Can't seem to use NVIC interrupts for TIM2 ch1 and TIM5 ch1, why?
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
