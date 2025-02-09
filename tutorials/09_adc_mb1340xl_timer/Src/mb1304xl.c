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
uint16_t envelope_buffer[100];

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

	// set counting direction
	TIM2->CR1 &= ~TIM_CR1_DIR;

	// set prescaler
	TIM2->PSC = TRIGGER_TIMER_PSC;

	// set Auto Reload Value, sets the PRI
	TIM2->ARR = TRIGGER_TIMER_ARR - 1;

	// set output compare register for channel 1
	TIM2->CCR1 = 3; // 5 clock cycles --> 5 * 10us/clock = 50us

	// select PWM mode for channel 1
	TIM2->CCMR1 &= ~TIM_CCMR1_OC1M_Msk; // clear bits
	TIM2->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2); // 110 - PWM mode 1

	// enable register preload
	TIM2->CCMR1 |= TIM_CCMR1_OC1PE;

	// slect output polarity to 0 - active high
	TIM2->CCER &= ~TIM_CCER_CC1P;

	// enable output of channel 1
	TIM2->CCER |= TIM_CCER_CC1E;

	// enable TIM2
	TIM2->CR1 |= TIM_CR1_CEN;

}


/* Setup PWM timer on TIM1 channel 1 for ranging trigger
 * desired PW is 30us with a desired PRI of 70ms
 */
void config_PWM_TIM5_ch1_trigger(void)
{
	// enable clock access to TIM2
	RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;

	// ensure TIM2 is disabled
	TIM5->CR1 &= ~TIM_CR1_CEN;

	// set counting direction
	TIM5->CR1 &= ~TIM_CR1_DIR;

	// set prescaler
	TIM5->PSC = TRIGGER_TIMER_PSC;

	// set Auto Reload Value, sets the PRI
	TIM5->ARR = TRIGGER_TIMER_ARR - 1;

	// set output compare register for channel 1
	TIM5->CCR1 = 5; // 3 clock cycles --> 3 * 10us/clock = 30us

	// select PWM mode for channel 1
	TIM5->CCMR1 &= ~TIM_CCMR1_OC1M_Msk; // clear bits
	TIM5->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2); // 110 - PWM mode 1

	// enable register preload
	TIM5->CCMR1 |= TIM_CCMR1_OC1PE;

	// slect output polarity to 0 - active high
	TIM5->CCER &= ~TIM_CCER_CC1P;

	// enable output of channel 1
	TIM5->CCER |= TIM_CCER_CC1E;

	// enable interrupt on channel 1
//	TIM5->DIER |= TIM_DIER_CC1IE;
	// TIM5->DIER |= TIM_DIER_UIE;

	// enable trigger event
//	TIM5->EGR |= TIM_EGR_CC1G;

//	// enable TIM5 interrupt on NVIC
//	NVIC_EnableIRQ(TIM5_IRQn);
//
//	// set interrupt priority
//	NVIC_SetPriority(TIM5_IRQn, 0); // highest priority

	// enable TIM5
	TIM5->CR1 |= TIM_CR1_CEN;

}



/* Setup PWM timer on TIM1 channel 4 for ranging trigger
 * desired PW is 50us with a desired PRI of 70ms
 */
void config_PWM_TIM2_ch4_adc_trigger(void)
{
	// enable clock access to TIM2
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	// ensure TIM2 is disabled
	TIM2->CR1 &= ~TIM_CR1_CEN;

	// set counting direction
	TIM2->CR1 &= ~TIM_CR1_DIR;

	// set prescaler
	TIM2->PSC = ADC_TIMER_PSC;

	// set Auto Reload Value, sets the PRI
	TIM2->ARR = ADC_TIMER_ARR - 1;

	// set output compare register for channel 4
	TIM2->CCR4 = 100; // 100 clock cycles --> 100 * 1 us/clock = 100us

	// select PWM mode for channel 4
	TIM2->CCMR2 &= ~TIM_CCMR2_OC4M_Msk; // clear bits
	TIM2->CCMR2 |= (TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2); // 110 - PWM mode 1

	// enable register preload
	TIM2->CCMR2 |= TIM_CCMR2_OC4PE;

	// select output polarity to 0 - active high
	TIM2->CCER &= ~TIM_CCER_CC4P;

	// enable output of channel 4
	TIM2->CCER |= TIM_CCER_CC4E;

	// enable TIM2
	TIM2->CR1 |= TIM_CR1_CEN;

}


/* --------------------------------------------------------------------------------
 * ADC Rx Functions
 * --------------------------------------------------------------------------------
 */
/** Configure ADC1 with DMA
 *  We will also configure the ADC to trigger on timer 2 channel 1 events
 * **/
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

	// configure sequence length to enable 2 conversions (one for each channel)
	ADC1->SQR1 &= ~ADC_SQR1_L_Msk; // reset to zero
	ADC1->SQR1 |= ADC_SQR1_L_0; // 2 channel conversion

	// set sequence
	ADC1->SQR3 |= ADC_SQR3_SQ1_0; // seq conversion 1, channel 1 for PA1
	ADC1->SQR3 |= ADC_SQR3_SQ2_2; // seq conversion 2, channel 2 for PA4

	// enable scan mode
	ADC1->CR1 |= ADC_CR1_SCAN;

	// enable external trigger on rising edge
	ADC1->CR2 |= ADC_CR2_EXTEN_0;
	ADC1->CR2 &= ~ADC_CR2_EXTEN_1;

//	// enable external trigger on falling edge
//	ADC1->CR2 &= ~ADC_CR2_EXTEN_0;
//	ADC1->CR2 |= ADC_CR2_EXTEN_1;

	// enable trigger TIM2 CC4 event
	ADC1->CR2 |= ADC_CR2_EXTSEL_0;
	ADC1->CR2 &= ~ADC_CR2_EXTSEL_1;
	ADC1->CR2 |= ADC_CR2_EXTSEL_2;
	ADC1->CR2 &= ~ADC_CR2_EXTSEL_3;

//	// enable trigger TIM5 CC1 event
//	ADC1->CR2 &= ~ADC_CR2_EXTSEL_0;
//	ADC1->CR2 |= ADC_CR2_EXTSEL_1;
//	ADC1->CR2 &= ~ADC_CR2_EXTSEL_2;
//	ADC1->CR2 |= ADC_CR2_EXTSEL_3;

	// enable single conversion
	ADC1->CR2 &= ~ADC_CR2_CONT;
	// ADC1->CR2 |= ADC_CR2_CONT; // continuous for debug

	// select to use DMA
	ADC1->CR2 |= (ADC_CR2_DMA | ADC_CR2_DDS);

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

	// reset register
	DMA2_Stream0->CR = 0x0;

	// enable circular mode (and reset register)
	DMA2_Stream0->CR = DMA_SxCR_CIRC;

	// disable circular mode
	// DMA2_Stream0->CR &= ~DMA_SxCR_CIRC;

	// channel 0 selected by default upon reset

	// set DMA2 Stream0 memory datasize to half word (16 bits)
	DMA2_Stream0->CR |= DMA_SxCR_MSIZE_0;
	DMA2_Stream0->CR &= ~DMA_SxCR_MSIZE_1;

	// set peripheral data size to half-word
	DMA2_Stream0->CR |= DMA_SxCR_PSIZE_0;
	DMA2_Stream0->CR &= ~DMA_SxCR_PSIZE_1;

	// enable memory increment (ensure we fill up adc data array)
	DMA2_Stream0->CR |= DMA_SxCR_MINC;

	// set peripheral address
	DMA2_Stream0->PAR = (uint32_t) &(ADC1->DR);

	// set memory address
	DMA2_Stream0->M0AR = (uint32_t) (&adc_data);

	// set number of transfers
	DMA2_Stream0->NDTR = (uint16_t) NUM_ADC_CHANNELS;

	// enable transfer complete interrupt
	DMA2_Stream0->CR |= DMA_SxCR_TCIE;

	// enable interrupt on NVIC
	NVIC_EnableIRQ(DMA2_Stream0_IRQn);

	// enable DMA stream
	DMA2_Stream0->CR |= DMA_SxCR_EN;

}


/** configure DMA 2 for memory to memory **/
void config_DMA2_mem2mem(void)
{
	/** DMA configuration **/
	// enable clock access to DMA module
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

	// disable DMA stream
	DMA2_Stream5->CR &= ~DMA_SxCR_EN;

	// wait until DMA stream is disabled
	while( DMA2_Stream5->CR & DMA_SxCR_EN ) {}

	// reset register
	DMA2_Stream5->CR = 0x0;

	// enable circular mode (and reset register)
	DMA2_Stream5->CR = DMA_SxCR_CIRC;

	// disable circular mode
	// DMA2_Stream0->CR &= ~DMA_SxCR_CIRC;

	// channel 0 selected by default upon reset
	// set channel 1

	// set transfer direction (Memory-to-Memory)
	DMA2_Stream5->CR &= ~DMA_SxCR_DIR_0;
	DMA2_Stream5->CR |= DMA_SxCR_DIR_1;

	// set DMA2 Stream0 memory datasize to half word (16 bits)
	DMA2_Stream5->CR |= DMA_SxCR_MSIZE_0;
	DMA2_Stream5->CR &= ~DMA_SxCR_MSIZE_1;

	// set peripheral data size to half-word
	DMA2_Stream5->CR |= DMA_SxCR_PSIZE_0;
	DMA2_Stream5->CR &= ~DMA_SxCR_PSIZE_1;

	// enable memory increment (ensure we fill buffer)
	DMA2_Stream5->CR |= DMA_SxCR_MINC;
	// DMA2_Stream5->CR |= DMA_SxCR_PINC;

	// set peripheral address
	DMA2_Stream5->PAR = (uint32_t) &adc_data[1];

	// set memory address
	DMA2_Stream5->M0AR = (uint32_t) (&envelope_buffer);

	// set number of transfers
	DMA2_Stream5->NDTR = (uint16_t) 100;

	// enable transfer complete interrupt
	DMA2_Stream5->CR |= DMA_SxCR_TCIE;

	// enable interrupt on NVIC
	NVIC_EnableIRQ(DMA2_Stream5_IRQn);

	// enable DMA stream
	DMA2_Stream5->CR |= DMA_SxCR_EN;
}

/* --------------------------------------------------------------------------------
 * Pin config Functions
 * --------------------------------------------------------------------------------
 */

/** configure PA0 for Timer 2 channel 1 usage**/
void config_PA0_AF1(void)
{
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// enable Alternate Function mode for PA0
	GPIOA->MODER &= ~GPIO_MODER_MODE0_Msk; // clear Mode bits
	GPIOA->MODER |= GPIO_MODER_MODE0_1;    // set mode bits to 10 for AF

	// set PA0 to AF1 for TIM2
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL0_Msk; // clear AFRL bits
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL0_0;     // set AFRL to 0001 for AF1

	// set Output Speed to low
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR0;

	// ensure PA0 has no pull-up or pull-down
	GPIOA->PUPDR &= GPIO_PUPDR_PUPD0_Msk;
}

/** configure PA0 for Timer 5 channel 1 usage**/
void config_PA0_AF2(void)
{
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// enable Alternate Function mode for PA0
	GPIOA->MODER &= ~GPIO_MODER_MODE0_Msk; // clear Mode bits
	GPIOA->MODER |= GPIO_MODER_MODE0_1;    // set mode bits to 10 for AF

	// set PA0 to AF2 for TIM5
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL0_Msk; // clear AFRL bits
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL0_1;     // set AFRL to 0002 for AF2

	// set Output Speed to low
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR0;

	// ensure PA0 has no pull-up or pull-down
	GPIOA->PUPDR &= GPIO_PUPDR_PUPD0_Msk;
}


/** configure PA0 for Timer 5 channel 1 usage**/
void config_PA3_AF1(void)
{
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// enable Alternate Function mode for PA3
	GPIOA->MODER &= ~GPIO_MODER_MODE3_Msk; // clear Mode bits
	GPIOA->MODER |= GPIO_MODER_MODE3_1;    // set mode bits to 10 for AF

	// set PA3 to AF1 for TIM2
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL3_Msk; // clear AFRL bits
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL3_0;     // set AFRL to 0001 for AF1

	// set Output Speed to low
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR3;

	// ensure PA3 has no pull-up or pull-down
	GPIOA->PUPDR &= GPIO_PUPDR_PUPD3_Msk;
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



