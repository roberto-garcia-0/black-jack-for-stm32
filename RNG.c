#include "RNG.h"
#include "SysTimer.h"
#include "stm32l476xx.h"
#include <stdio.h>

volatile uint32_t random;

void RNG_Init() {
  RCC->CR &= ~RCC_CR_MSIRANGE; // reset MSI range
  RCC->CR |= RCC_CR_MSIRANGE_11; // set MSI range to 0b1011 (48 MHz)

  // Use the MSI clock range that is defined in RCC_CR
  RCC->CR |= RCC_CR_MSIRGSEL; // select range defined in MSIRANGE under RCC->CR

  // Enable MSI oscillator
  RCC->CR |= RCC_CR_MSION;

  RCC->CCIPR |= RCC_CCIPR_CLK48SEL;
  RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;


  RNG->CR |= RNG_CR_IE;
  // RNG->CR |= RNG_CR_RNGEN;
  // while (!(RNG->SR & RNG_SR_DRDY));
  NVIC_EnableIRQ(RNG_IRQn);
  NVIC_SetPriority(RNG_IRQn, 2);
}

// this function enables the RNG and generates an interrupt. This will cause
// random to get a new hardware randomized value
uint32_t RNG_Get_Val() {
  RNG->CR |= RNG_CR_RNGEN;
  delay(1);
  return random;
}


void RNG_IRQHandler(void) {
  if (RNG->SR & RNG_SR_SEIS) {
    // seed error, requires reset of RNG
    RNG->CR &= ~RNG_CR_RNGEN;
    RNG->CR |= RNG_CR_RNGEN;
    RNG->SR &= ~RNG_SR_SEIS; // clear interrupt
  } else if (RNG->SR & RNG_SR_CEIS) {
    // clock error, so reset clock
    RNG->CR &= ~(RNG_CR_RNGEN);
    RCC->AHB2RSTR |= RCC_AHB2RSTR_RNGRST;
    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_RNGRST;
    RNG->CR |= RNG_CR_RNGEN;
    RNG->SR &= ~RNG_SR_CEIS; // clear interrupt
  } else {
    // Data is ready
    // Disabled RNG then read the data
    RNG->CR &= ~RNG_CR_RNGEN;
    random = RNG->DR;
  }



}
