/*
 * check out this other cool tutorial!
 * https://controllerstech.com/dma-with-adc-using-registers-in-stm32/
 *
 */

#include "adc_dma.h"


#define NUM_SEQ_CONVERSIONS   (5U << 20) // 6 sequence conversions

#define SEQ1_CHANNEL_0        ADC_SQR3_SQ1_Pos
#define SEQ2_CHANNEL_1        ADC_SQR3_SQ2_0
#define SEQ3_CHANNEL_3        ADC_SQR3_SQ3_2
#define SEQ4_CHANNEL_4        ADC_SQR3_SQ4_3
#define SEQ5_CHANNEL_10       (10 << 20)
#define SEQ6_CHANNEL_11       (11 << 25)


// why can't we put this in the .c file??
uint16_t adc_raw_data[NUM_CHANNELS];

// prototypes
void init_dma2_for_adc1(void);


// init ADC1 and DMA2 Stream0 channel 0
void adc_dma_init_6channel(void)
{
	/** GPIO config **/
	// enable clock access to ADC GPIO Pins Port
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

	// (CHANGE FOR DIFFERENT NUM OF CHANNELS)
	// set PA0, PA1, PA4 to analog mode
	GPIOA->MODER |= GPIO_MODER_MODE0_Msk; // PA0 as channel 0 on A0
	GPIOA->MODER |= GPIO_MODER_MODE1_Msk; // PA1 as channel 1 on A1
    GPIOA->MODER |= GPIO_MODER_MODE4_Msk; // PA4 as channel 4 on A2
    GPIOB->MODER |= GPIO_MODER_MODE0_Msk; // PB0 as channel 8 on A3
    GPIOC->MODER |= GPIO_MODER_MODE1_Msk; // PC1 as channel 11 on A4
    GPIOC->MODER |= GPIO_MODER_MODE0_Msk; // PC0 as channel 10 on A5

	/** ADC config **/
	// enable clock access to ADC1
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

	// (CHANGE FOR DIFFERENT NUM OF CHANNELS)
	// configure sequence length to enable 3 conversions (one for each channel)
	ADC1->SQR1 &= ~ADC_SQR1_L_Msk; // reset to zero
	ADC1->SQR1 |= NUM_SEQ_CONVERSIONS; // 6 channels

	// set sequence
	// (CHANGE FOR DIFFERENT NUM OF CHANNELS)
	// ADC1->SQR3 = (0U << 0) | (1U << 5);
	ADC1->SQR3 |= SEQ1_CHANNEL_0;  // sequence conversion 1, channel 0
	ADC1->SQR3 |= SEQ2_CHANNEL_1;  // sq 2, channel 1
	ADC1->SQR3 |= SEQ3_CHANNEL_3;  // sq3, channel 4
	ADC1->SQR3 |= SEQ4_CHANNEL_4;  // sq4, channel 8
	ADC1->SQR3 |= SEQ5_CHANNEL_10; // seq 5, chan 10
	ADC1->SQR3 |= SEQ6_CHANNEL_11; // seq 6, chan 11

	// enable scan mode
	ADC1->CR1 |= ADC_CR1_SCAN;

	// select to use DMA
	ADC1->CR2 |= (ADC_CR2_CONT | ADC_CR2_DMA | ADC_CR2_DDS);

	// configure DMA2 for ADC1
	init_dma2_for_adc1();

	/** Finish configuring the ADC **/
	// enable ADC
	ADC1->CR2 |= ADC_CR2_ADON;

	//start ADC
	ADC1->CR2 |= ADC_CR2_SWSTART;

}


void adc_dma_init(void)
{
	/** GPIO config **/
	// enable clock access to ADC GPIOA Pins Port
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// (CHANGE FOR DIFFERENT NUM OF CHANNELS)
	// set PA0, PA1, PA4 to analog mode
	GPIOA->MODER |= GPIO_MODER_MODE0_Msk; // PA0
	GPIOA->MODER |= GPIO_MODER_MODE1_Msk; // PA1
    GPIOA->MODER |= GPIO_MODER_MODE4_Msk; // PA4

	/** ADC config **/
	// enable clock access to ADC1
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

	// (CHANGE FOR DIFFERENT NUM OF CHANNELS)
	// configure sequence length to enable 3 conversions (one for each channel)
	ADC1->SQR1 &= ~ADC_SQR1_L_Msk; // reset to zero
	ADC1->SQR1 |= ADC_SQR1_L_1; // 3 channel conversion!!


	// set sequence
	// (CHANGE FOR DIFFERENT NUM OF CHANNELS)
	ADC1->SQR3 |= ADC_SQR3_SQ1_Pos; // channel 0
	ADC1->SQR3 |= ADC_SQR3_SQ2_0;   // channel 1
	ADC1->SQR3 |= ADC_SQR3_SQ3_2; // sequence conversion 3, channel 4

	// enable scan mode
	ADC1->CR1 |= ADC_CR1_SCAN;

	// select to use DMA
	ADC1->CR2 |= (ADC_CR2_CONT | ADC_CR2_DMA | ADC_CR2_DDS);

	// configure DMA2 for ADC1
	init_dma2_for_adc1();

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




void init_dma2_for_adc1(void)
{
	/** DMA configuration **/
	// enable clock access to DMA module
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

	// disable DMA stream
	DMA2_Stream0->CR &= ~DMA_SxCR_EN;

	// wait until DMA stream is disabled
	while( DMA2_Stream0->CR & DMA_SxCR_EN ) {}

	// enable circular mode
	DMA2_Stream0->CR |= DMA_SxCR_CIRC;

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
	DMA2_Stream0->M0AR = (uint32_t) (&adc_raw_data);

	// set number of transfers
	DMA2_Stream0->NDTR = (uint16_t) NUM_CHANNELS;

	// enable DMA stream
	DMA2_Stream0->CR |= DMA_SxCR_EN;

}

