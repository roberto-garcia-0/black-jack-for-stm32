#ifndef __STM32L476R_NUCLEO_SPI_H
#define __STM32L476R_NUCLEO_SPI_H

#include "stm32l476xx.h"

void SPI1_GPIO_Init(void);
void SPI1_Init(void);
uint8_t SPI_Transfer_Byte(SPI_TypeDef* SPIx, uint8_t write_data);


#endif
