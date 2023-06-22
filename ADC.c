#include "ADC.h"

#include "stm32l476xx.h"

#include <stdint.h>

void ADC_Wakeup(void) {
    int wait_time;

    // To start ADC operations, the following sequence should be applied
    // DEEPPWD = 0: ADC not in deep-power down
    // DEEPPWD = 1: ADC in deep-power-down (default reset state)
    if ((ADC1->CR & ADC_CR_DEEPPWD) == ADC_CR_DEEPPWD)
        ADC1->CR &= ~ADC_CR_DEEPPWD; // Exit deep power down mode if still in that state

    // Enable the ADC internal voltage regulator
    // Before performing any operation such as launching a calibration or enabling the ADC, the ADC
    // voltage regulator must first be enabled and the software must wait for the regulator start-up
    // time.
    ADC1->CR |= ADC_CR_ADVREGEN;

    // Wait for ADC voltage regulator start-up time
    // The software must wait for the startup time of the ADC voltage regulator (T_ADCVREG_STUP)
    // before launching a calibration or enabling the ADC.
    // T_ADCVREG_STUP = 20 us
    wait_time = 20 * (80000000 / 1000000);
    while (wait_time != 0) {
        wait_time--;
    }
}

void ADC_Common_Configuration() {
  // enable booster for voltage switch
  SYSCFG->CFGR1 |= SYSCFG_CFGR1_BOOSTEN;

  // enable VrefInt
  ADC123_COMMON->CCR |= ADC_CCR_VREFEN;
  // configure to clk not divided (0b0000)
  ADC123_COMMON->CCR &= ~ADC_CCR_PRESC;
  // config to ADCs clk mode to HCLK/1 (0b01)
  ADC123_COMMON->CCR &= ~ADC_CCR_CKMODE;
  ADC123_COMMON->CCR |= ADC_CCR_CKMODE_0;
  // confige all ADCs to independent mode (0b00000)
  ADC123_COMMON->CCR &= ~ADC_CCR_DUAL;



}

void ADC_Pin_Init(void) {
    // enable clock for PA
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    // analog mode (0b11) PA1
    GPIOA->MODER |= GPIO_MODER_MODE1;
    // no pull up/pull down PA1
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD1;
    // connect analog switch to PA1
    GPIOA->ASCR |= GPIO_ASCR_ASC1;

}

void ADC_Init(void) {
    // enable ADC clk
    RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
    // reset and clear ADC reset reg
    RCC->AHB2RSTR |= RCC_AHB2RSTR_ADCRST;
    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_ADCRST;

    // Other ADC Initialization
    ADC_Pin_Init();
    ADC_Common_Configuration();
    ADC_Wakeup();

    // disable ADC1
    ADC1->CR &= ~ADC_CR_ADEN;

    // config ADC1 to 12-bit res (0b00)
    ADC1->CFGR &= ~ADC_CFGR_RES;
    // config to right-align
    ADC1->CFGR &= ~ADC_CFGR_ALIGN;

    // config sequence length to one (0b0000)
    ADC1->SQR1 &= ~ADC_SQR1_L;
    // assign channel 6 to first sequence conversion
    ADC1->SQR1 &= ~ADC_SQR1_SQ1; // reset bits
    ADC1->SQR1 |= (ADC_SQR1_SQ1_2 | ADC_SQR1_SQ1_1); // set bits to 6 (0b00110)

    // config channel 6 to single ended mode
    ADC1->DIFSEL &= ~ADC_DIFSEL_DIFSEL_6;

    // config channel 6 sampling time to 24.5 ADC clk cycles (0b011)
    ADC1->SMPR1 &= ~ADC_SMPR1_SMP6;
    ADC1->SMPR1 |= (ADC_SMPR1_SMP6_1 | ADC_SMPR1_SMP6_0);

    // config ADC1 to single conversion mode
    ADC1->CFGR &= ~ADC_CFGR_CONT;
    // disable ADC1 hardware trigger detection (0b00)
    ADC1->CFGR &= ~ADC_CFGR_EXTEN;

    // enable ADC1
    ADC1->CR |= ADC_CR_ADEN;
    // wait until ready
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);
}

uint16_t ADC_Get_Reading() {
  // start regualar conversion
  ADC1->CR |= ADC_CR_ADSTART;
  // wait until conversion is complete
  while ((ADC123_COMMON->CSR & ADC_CSR_EOC_MST) == 0);
  // read and return data
  return (ADC1->DR & ADC_DR_RDATA);
}
