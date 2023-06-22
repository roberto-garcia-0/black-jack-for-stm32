#include "SPI.h"

/*
PA4 - SS GPO (connected to CE)
  Note: This pin will be controlled manually though software

PA5 - SPI1_SCK (connected to CLK)
PA6 - SPI1_MISO (No use in this case)
PA7 - SPI1_MOSI (connected to DIN)
*/

void SPI1_GPIO_Init(void) {
  // enable clock for port
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

  GPIOA->MODER &= ~(GPIO_MODER_MODE4 | GPIO_MODER_MODE5 | GPIO_MODER_MODE6 | GPIO_MODER_MODE7);
  // set mode of pins to alternating functions (5, 6, 7)(0b10);
  // set mode of PA4 to GPO  (0b01)
  GPIOA->MODER |= GPIO_MODER_MODE4_0 | GPIO_MODER_MODE5_1 | GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1;

  // set pins 5, 6, 7 to alternating function AF5 (0b0101)
  GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL5 | GPIO_AFRL_AFSEL6 | GPIO_AFRL_AFSEL7) ;
  GPIOA->AFR[0] |=  (GPIO_AFRL_AFSEL5_0 | GPIO_AFRL_AFSEL5_2 | GPIO_AFRL_AFSEL6_0 | GPIO_AFRL_AFSEL6_2 | GPIO_AFRL_AFSEL7_0 | GPIO_AFRL_AFSEL7_2);

  // set all pins to very high output speed (0b11)
  GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED4 | GPIO_OSPEEDR_OSPEED5 | GPIO_OSPEEDR_OSPEED6 | GPIO_OSPEEDR_OSPEED7;

  // configure output push-pull (0b0)
  GPIOA->OTYPER &= ~(GPIO_OTYPER_OT4 | GPIO_OTYPER_OT5 | GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7);

  // configure to no pull I/O type (0b00)
  GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD4 | GPIO_PUPDR_PUPD5 | GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD7);

  // set NSS(4) pin to high
  GPIOA->ODR |= GPIO_ODR_OD4;
}

void SPI1_Init(void){
  // enable SPI1 clk
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

  // reset peripheral
  RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
  RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST;

  // configure SPI1 to master mode (0b1)
  SPI1->CR1 |= SPI_CR1_MSTR;

  // configure baud rate prescaler to 8 -> 16MHz/8 = 2 MHz (0b010)
  // SPI1->CR1 &= ~SPI_CR1_BR;
  SPI1->CR1 |= SPI_CR1_BR;
  SPI1->CR1 &= ~(SPI_CR1_BR_2 | SPI_CR1_BR_0);

  // enable slave select output
  SPI1->CR2 |= SPI_CR2_SSOE;

  SPI1->CR2 |= SPI_CR2_FRXTH;

  // renable SPI
  SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t SPI_Transfer_Byte(SPI_TypeDef* SPIx, uint8_t write_data) {
  while (!(SPIx->SR & SPI_SR_TXE));
  GPIOA->ODR &= ~GPIO_ODR_OD4;
  *((volatile uint8_t*) &(SPIx->DR)) = write_data;
  // SPIx->DR = write_data;
  while (SPIx->SR & SPI_SR_BSY);
  GPIOA->ODR |= GPIO_ODR_OD4;
  return *((volatile uint8_t*) &(SPIx->DR));

}
