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
const float CLOCKS_2_SEC = TRIGGER_TIMER_PSC / (float) SYS_FREQ;
const float CLOCKS_2_US =  (TRIGGER_TIMER_PSC / (float) SYS_FREQ) * 1000000;
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

	// configure ADC trigger timer
	config_PWM_TIM2_ch4_adc_trigger();

	// configure ADC with DMA for 2 inputs into PA1 and PA4
	config_adc1_dma();

	config_DMA2_mem2mem();

	// config PA0 as the trigger pin
	config_PA0_AF2();
	config_PWM_TIM5_ch1_trigger();



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
		// stop ADC --> This doesn't seem to be working
        // ADC1->CR2 &= ~ADC_CR2_SWSTART;

        // disable DMA stream
        // DMA2_Stream0->CR &= ~DMA_SxCR_EN;

        // wait until DMA stream is disabled
        // while( DMA2_Stream0->CR & DMA_SxCR_EN ) {}

        // printf("%lu %d %d \n", TIM2->CNT, 0, 0);
        // printf("%lu %d %d \n", TIM5->CNT, adc_data[0], adc_data[1]);

	    // STOP ADC continuous transfer
		// ADC1->CR2 &= ~ADC_CR2_CONT;

//		// re-enable ADC conversion
//		ADC1->CR2 |= ADC_CR2_CONT;

		// print raw values
//		for (int i = 0; i < NUM_ADC_CHANNELS; i += 2)
//		{
//			printf("%lu %d %d \n", TIM5->CNT, adc_data[i], adc_data[i+1]);
//		}
//
//		printf("\n\n\n\n");
//
//		for (int i = 0; i < 10; i++)
//		{
//			printf("%lu %d %d \n", TIM5->CNT, 0, 0);
//		}

//		// enable ADC
//		ADC1->CR2 |= ADC_CR2_ADON;

//		// STOP ADC conversion
//		ADC1->CR2 &= ~ADC_CR2_CONT;

//		// re-enable ADC conversion
//		ADC1->CR2 |= ADC_CR2_CONT;

	}

	// clear flags
	DMA2->LIFCR |= (DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0);
}


/** DMA interrupt request handler **/
void DMA2_Stream5_IRQHandler(void)
{
	// check if DMA transfer has completed
	if ((DMA2->HISR & DMA_HISR_TCIF5) == DMA_HISR_TCIF5)
	{
        // printf("%lu %d %d \n", TIM2->CNT, 0, 0);
        // printf("%lu %d %d \n", TIM5->CNT, adc_data[0], adc_data[1]);

        for (int i = 0; i < 100; i++)
		{
			printf("%lu %d %d \n", TIM5->CNT, 0, envelope_buffer[i]);
		}

        for (int i = 0; i < 10; i++)
        {
        	printf("%d %d %d \n", 0, 0, 0);
        }

	}

	// clear flags
	DMA2->HIFCR |= (DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5);

	// re enable stream5
	DMA2_Stream5->CR |= DMA_SxCR_EN;
}


/* IRQ Handler for TIM5 channel 1
 *
 * Maybe use this to start the ADC conversion timer?
 * */
void TIM5_IRQHandler(void)
{
	// if ((TIM5->SR & TIM_SR_UIF) != 0)
	if ((TIM5->SR & TIM_SR_CC1IF) != 0)
	{
		// clear update interrupt flag
		// TIM5->SR &= ~TIM_SR_UIF;
		TIM5->SR &= ~TIM_SR_CC1IF;

		// enable continous transfer??
		// ADC1->CR2 |= ADC_CR2_CONT;

		//start ADC
		ADC1->CR2 |= ADC_CR2_SWSTART;

		// enable DMA stream
		// DMA2_Stream0->CR |= DMA_SxCR_EN;

		printf("%lu %d %d \n", TIM5->CNT, 20, 20);
	}
}

