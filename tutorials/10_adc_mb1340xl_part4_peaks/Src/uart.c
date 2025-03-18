#include "uart.h"


#define UART_BAUDRATE     115200
#define SYS_CLK           16000000


/** function prototypes **/
static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate);
void uart2_write(int ch);

int __io_putchar(int ch)
{
	uart2_write(ch);

	return ch;
}



void uart2_tx_init(void)
{
	// enable clock access to GPIOA
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// set PA2 to alternate function mode
	GPIOA->MODER &= ~GPIO_MODER_MODE2_0;
	GPIOA->MODER |= GPIO_MODER_MODE2_1;

	// set PA2 AF type to AF7 for USART2
	GPIOA->AFR[0] |= GPIO_AFRL_AFRL2;
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL2_3;

	// enable clock access to USART2
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

	// set baud rate
	uart_set_baudrate((uint32_t) SYS_CLK, (uint32_t) UART_BAUDRATE);

	// set Tx direction
	USART2->CR1 |= USART_CR1_TE;

	// enable uart module
	USART2->CR1 |= USART_CR1_UE;

}


/*
 * DMA1 channel 4
 *     stream 5 - Rx, stream 6 - Tx
 */
void uart2_write(int ch)
{
	// ensure tx data register is empty
	while( !(USART2->SR & USART_SR_TXE) ) {}

	// write data to first 8 bits of data register
	USART2->DR = (ch & 0xFF);

}



/** Baudrate functions **/
static uint16_t compuate_uart_bd(uint32_t periph_clk, uint32_t baudrate)
{
	return (periph_clk + (baudrate / 2U))/baudrate;
}


static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate)
{
	USART2->BRR = compuate_uart_bd(periph_clk, baudrate);
}

