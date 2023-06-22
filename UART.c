#include "UART.h"
#include "SysTimer.h"
#include <stdio.h>

void UART1_Init(void) {
	// enable clock for USART1
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

	// select clock source for USART1 as sys clock (0b01)
	RCC->CCIPR &= ~RCC_CCIPR_USART1SEL;
	RCC->CCIPR |= RCC_CCIPR_USART1SEL_0;

}

void UART1_GPIO_Init(void) {
	// enable clock for port B
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

	// set mode of pins to alternating functions (0b10); PB6 and PB7
	GPIOB->MODER &= ~GPIO_MODER_MODE6;
	GPIOB->MODER |= GPIO_MODER_MODE6_1;
	GPIOB->MODER &= ~GPIO_MODER_MODE7;
	GPIOB->MODER |= GPIO_MODER_MODE7_1;

	// set the alternating function for PB6 and PB7; both should be set to AF7 (0b0111)
	GPIOB->AFR[0] &= ~GPIO_AFRL_AFSEL6;
	GPIOB->AFR[0] |=  (GPIO_AFRL_AFSEL6_2 | GPIO_AFRL_AFSEL6_1 | GPIO_AFRL_AFSEL6_0);
	GPIOB->AFR[0] &= ~GPIO_AFRL_AFSEL7;
	GPIOB->AFR[0] |=  (GPIO_AFRL_AFSEL7_2 | GPIO_AFRL_AFSEL7_1 | GPIO_AFRL_AFSEL7_0);

	// set to very high output speed
	GPIOB->OSPEEDR |= GPIO_OSPEEDR_OSPEED6;
	GPIOB->OSPEEDR |= GPIO_OSPEEDR_OSPEED7;

	// reset to output push-pull
	GPIOB->OTYPER &= ~GPIO_OTYPER_OT6;
	GPIOB->OTYPER &= ~GPIO_OTYPER_OT7;

	// configure to pull-up I/O type (0b01)
	GPIOB->PUPDR &= ~GPIO_PUPDR_PUPD6;
	GPIOB->PUPDR |= GPIO_PUPDR_PUPD6_0;
	GPIOB->PUPDR &= ~GPIO_PUPDR_PUPD7;
	GPIOB->PUPDR |= GPIO_PUPDR_PUPD7_0;
}

void USART_Init(USART_TypeDef* USARTx) {
	// make USART disabled before initializing
	USARTx->CR1 &= ~USART_CR1_UE;

	// configure to word length of 8 bits (0b00)
	USARTx->CR1 &= ~USART_CR1_M;
	// configure oversampling to 16 (0b0)
	USARTx->CR1 &= ~USART_CR1_OVER8;
	// configure to one stop bit (0b00)
	USARTx->CR2 &= ~USART_CR2_STOP;

	// SYS CLK is 16Mhz, so 9600 = 16MHz / USARTDIV -> USARTDIV ~ 1667 -> 0x683
	USARTx->BRR = 0x683U;

	// enable both RX and TX
	USARTx->CR1 |= USART_CR1_TE;
	USARTx->CR1 |= USART_CR1_RE;

	// renable USART
	USARTx->CR1 |= USART_CR1_UE;
}

uint8_t USART_Read (USART_TypeDef * USARTx) {

	// SR_RXNE (Read data register not empty) bit is set by hardware
	while (!(USARTx->ISR & USART_ISR_RXNE));  // Wait until RXNE (RX not empty) bit is set
	// USART resets the RXNE flag automatically after reading DR

	return ((uint8_t)(USARTx->RDR & 0xFF));
	// Reading USART_DR automatically clears the RXNE flag
}

unsigned int USART_Read_Str_to_Int(uint8_t size) {
  uint8_t i = 0;
	unsigned int res = 0;
	unsigned int factor = 1;
	for (size_t j = 0; j < size - 1; j++) {
		factor *= 10;
	}
  while (i != size) {
    switch (USART_Read(USART1)) {
    	case '9':
				res += 9 * factor;
				break;
    	case '8':
				res += 8 * factor;
				break;
    	case '7':
				res += 7 * factor;
				break;
    	case '6':
				res += 6 * factor;
				break;
    	case '5':
				res += 5 * factor;
				break;
    	case '4':
				res += 4 * factor;
				break;
    	case '3':
				res += 3 * factor;
				break;
    	case '2':
				res += 2 * factor;
				break;
    	case '1':
				res += factor;
				break;
    	case '0':
			default:
				break;
    }
		factor /= 10;
    i++;
  }
	USART_Read(USART1); // trash reading
	return res;

}

void USART_Write(USART_TypeDef * USARTx, uint8_t *buffer, uint32_t nBytes) {
	int i;
	// TXE is cleared by a write to the USART_DR register.
	// TXE is set by hardware when the content of the TDR
	// register has been transferred into the shift register.
	for (i = 0; i < nBytes; i++) {
		while (!(USARTx->ISR & USART_ISR_TXE));   	// wait until TXE (TX empty) bit is set
		// Writing USART_DR automatically clears the TXE flag
		USARTx->TDR = buffer[i] & 0xFF;
		USART_Delay(60);
	}
	while (!(USARTx->ISR & USART_ISR_TC));   		  // wait until TC bit is set
	USARTx->ISR &= ~USART_ISR_TC;
}

void USART_Delay(uint32_t us) {
	uint32_t time = 100*us/7;
	while(--time);
}
