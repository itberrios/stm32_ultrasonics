/*
 * mb1304xl.c
 *
 *  Created on: Oct 24, 2024
 *      Author: iberrios
 */


#include "mb1304xl.h"

/** prototypes **/
void config_dma2_ch0_stream0_adc1(void);

/** globals **/
uint16_t adc_data[NUM_ADC_CHANNELS];

/* Determine ADC Voltage Scaling in mV: Vcc/4095 */
const float ADC_VOLTAGE_SCALE = (VCC == 33) ? 0.8058608059 : 1.221001221; // mV/sample

/* determine sensor voltage scaling: Vcc/1024
 * 3.3V --> 3.2mV/cm
 * 5V --> 4.9mV/cm
 */
const float SENSOR_VOLTAGE_SCALE = (VCC == 33) ? 3.22265625 : 4.8828125; // mV/cm

// conversion factor from ADC Samples to measured distance in cm
const float SAMPLES_TO_CM = ADC_VOLTAGE_SCALE / SENSOR_VOLTAGE_SCALE;

/* --------------------------------------------------------------------------------
 * PWM Trigger config Functions
 * --------------------------------------------------------------------------------
 */

/* Setup PWM timer on TIM1 channel 1 for ranging trigger
 * desired PW is 50us with a desired PRI of 70ms
 */
void config_PWM_TIM2_ch1_trigger(void)
{
	// enable clock access to TIM2
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	// ensure TIM2 is disabled
	TIM2->CR1 &= ~TIM_CR1_CEN;

	// set counting direction to upcounting mode
	TIM2->CR1 &= ~TIM_CR1_DIR;

	// set prescaler
	TIM2->PSC = TRIGGER_TIMER_PSC;

	// set Auto Reload Value, sets the PRI
	TIM2->ARR = TRIGGER_TIMER_ARR - 1;

	// set output compare register for channel 2
	TIM2->CCR1 = 100; // 5; // 5 clock cycles --> 5 * 10us/clock = 50us pulse width

	// select PWM mode for channel 2
	TIM2->CCMR1 &= ~TIM_CCMR1_OC1M_Msk; // clear bits
	TIM2->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2); // 110 - PWM mode 1

	// enable register preload
	TIM2->CCMR1 |= TIM_CCMR1_OC1PE;

	// slect output polarity to 0 - active high
	TIM2->CCER &= ~TIM_CCER_CC1P;

	// initialize all registers??
	// TIM2->EGR |= TIM_EGR_UG;

	// enable output of channel 1
	TIM2->CCER |= TIM_CCER_CC1E;

	// enable TIM2
	TIM2->CR1 |= TIM_CR1_CEN;

}

/* --------------------------------------------------------------------------------
 * ADC Rx Functions
 * --------------------------------------------------------------------------------
 */
/** Configure ADC1 with DMA **/
void config_adc1_dma(void)
{
	/** GPIO config **/
	// enable clock access to ADC GPIOA Pins Port
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// (CHANGE FOR DIFFERENT NUM OF CHANNELS)
	// set PA0, PA1, PA4 to analog mode
	// GPIOA->MODER |= GPIO_MODER_MODE0_Msk; // PA0
	GPIOA->MODER |= GPIO_MODER_MODE1_Msk; // PA1 -> A1
    GPIOA->MODER |= GPIO_MODER_MODE4_Msk; // PA4 -> A2

	/** ADC config **/
	// enable clock access to ADC1
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

	// (CHANGE FOR DIFFERENT NUM OF CHANNELS)
	// configure sequence length to enable 3 conversions (one for each channel)
	ADC1->SQR1 &= ~ADC_SQR1_L_Msk; // reset to zero
	// ADC1->SQR1 |= ADC_SQR1_L_1; // 3 channel conversion!!
	ADC1->SQR1 |= ADC_SQR1_L_0; // 2 channel conversion


	// set sequence
	// (CHANGE FOR DIFFERENT NUM OF CHANNELS)
	// ADC1->SQR3 |= ADC_SQR3_SQ1_Pos; // conversion 1 ,channel 0 for PA0
	// ADC1->SQR3 |= ADC_SQR3_SQ2_0; // conversion 2, channel 1 for PA1
	// ADC1->SQR3 |= ADC_SQR3_SQ3_2; // sequence conversion 3, channel 4 for PA4

//	ADC1->SQR3 |= ADC_SQR3_SQ1_0; // seq conversion 1, channel 1 for PA1
//	ADC1->SQR3 |= ADC_SQR3_SQ2_1; // seq conversion 2, channel 2 for PA4

	ADC1->SQR3 |= ADC_SQR3_SQ1_0; // seq conversion 1, channel 1 for PA1
	ADC1->SQR3 |= ADC_SQR3_SQ2_2; // seq conversion 2, channel 2 for PA4

	// enable scan mode
	ADC1->CR1 |= ADC_CR1_SCAN;

	// select to use DMA
	ADC1->CR2 |= (ADC_CR2_CONT | ADC_CR2_DMA | ADC_CR2_DDS);

	// configure DMA2 for ADC1
	config_dma2_ch0_stream0_adc1();

	/** Finish configuring the ADC **/
	// enable ADC
	ADC1->CR2 |= ADC_CR2_ADON;

	// let ADC stabilize
	uint32_t wait_time = (uint32_t) (ADC_T_STAB/1000000) * SYS_CLOCK;
	while (wait_time > 0)
	{
		wait_time--;
	}

	//start ADC
	ADC1->CR2 |= ADC_CR2_SWSTART;

}



/** Initialize and configure DMA2 for ADC1 **/
void config_dma2_ch0_stream0_adc1(void)
{
	/** DMA configuration **/
	// enable clock access to DMA module
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

	// disable DMA stream
	DMA2_Stream0->CR &= ~DMA_SxCR_EN;

	// wait until DMA stream is disabled
	while( DMA2_Stream0->CR & DMA_SxCR_EN ) {}

	// enable circular mode (and reset register)
	DMA2_Stream0->CR = DMA_SxCR_CIRC;

	// set DMA2 Stream0 memory datasize to half word (16 bits)
	DMA2_Stream0->CR |= DMA_SxCR_MSIZE_0;
	DMA2_Stream0->CR &= ~DMA_SxCR_MSIZE_1;

	// set peripheral data size to half-word
	DMA2_Stream0->CR |= DMA_SxCR_PSIZE_0;
	DMA2_Stream0->CR &= ~DMA_SxCR_PSIZE_1;

	// enable memory increment
	DMA2_Stream0->CR |= DMA_SxCR_MINC;

	// set peripheral address
	DMA2_Stream0->PAR = (uint32_t) &(ADC1->DR);

	// set memory address
	DMA2_Stream0->M0AR = (uint32_t) (&adc_data);

	// set number of transfers
	DMA2_Stream0->NDTR = (uint16_t) NUM_ADC_CHANNELS;

	// enable DMA stream
	DMA2_Stream0->CR |= DMA_SxCR_EN;

}


/* --------------------------------------------------------------------------------
 * Pin config Functions
 * --------------------------------------------------------------------------------
 */

/** configure PA0 for Timer 5 channel 1 usage**/
void config_PA0_AF1(void)
{
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// enable Alternate Function mode for PA0
	GPIOA->MODER &= ~GPIO_MODER_MODE0_Msk; // clear Mode bits
	GPIOA->MODER |= GPIO_MODER_MODE0_1;    // set mode bits to 10 for AF

	// set PA0 to AF2 for TIM5
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL0_Msk; // clear AFRL bits
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL0_0;     // set AFRL to 0001 for AF1

	// set Output Speed to low
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR0;

	// ensure PA0 has no pull-up or pull-down
	GPIOA->PUPDR &= GPIO_PUPDR_PUPD0_Msk;
}

/** configure PA1 for ADC1 Channel 1 ***/
void config_PA1_ADC1_1(void)
{
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// enable Analog Mode for PA1
	GPIOA->MODER |= GPIO_MODER_MODE1_Msk; // set mode bits to 11 for Analog
}


/** configure PA2 for ADC1 Channel 2 ***/
void config_PA2_ADC1_2(void)
{
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// enable Analog Mode for PA1
	GPIOA->MODER |= GPIO_MODER_MODE2_Msk; // set mode bits to 11 for Analog
}



