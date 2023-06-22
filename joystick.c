#include "stm32l476xx.h"
#include "joystick.h"
#include "SysTimer.h"
#include "ADC.h"
#include "blackjack.h"

static volatile uint8_t joystick = 2;
static volatile uint8_t joystickSW;
// polls until a valid input is read from ADC (joystick)
// should be used with delay so that repeated calls don't instantly change
// options in game. It also accounts when an input at the SW is detected
// returns 0 - left, 1 - right, 2 - center/button pressed
char Joystick_Poll_Reading() {
  uint16_t reading;
  do {
    if (joystickSW) {
      return 2;
    }
    delay(20);
    reading = ADC_Get_Reading();
  } while((reading < 2547) && (reading > 1319));

  return reading > 1933;
}

void JoystickSW_EXTI_Init() {
  // Initialization of controllers/ports
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // Enable SYSCFG controller for EXTI for GPIO
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; // Set GPIO Port A clock enable

	// Initialize User Button (Configured to PIN 12, PORT A)

	// Set to input mode
	GPIOA->MODER &= ~GPIO_MODER_MODE12; // Reset GPIOC register mode of pin 12

  // Set to No Pull-Up, No Pull-Down
  GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD12;

	// Configure SYSCFG EXTI
	// This connects the PIN and PORT to the EXTI Line respective to the PIN
	// Only necessary for GPIO lines
	SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI12; 	 // Reset EXTI12 register
	SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI12_PA; // Set to enable PIN 12 for PORT A

	// Configure EXTI to Trigger on rising edge
	EXTI->FTSR1 |= EXTI_FTSR1_FT12; // reset EXTI12 trigger for falling edges
	EXTI->RTSR1 &= ~EXTI_RTSR1_RT12; // set EXTI12 trigger for rising edges

	// Enable EXTI
	EXTI->IMR1 |= EXTI_IMR1_IM12; // unmask (enable) interrupt line 12

	// Configure and Enable in NVIC
	NVIC_EnableIRQ(EXTI15_10_IRQn);
	NVIC_SetPriority(EXTI15_10_IRQn, 1);
}

char JoystickSW_Get() {
  char res = joystickSW;
  if (joystickSW) {
    joystickSW = 0;
  }
  return res;
}

char Joystick_Get(void) {
  char res = joystick;
  if (joystick != 2) {
    joystick = 2;
  }
  return res;
}


void EXTI15_10_IRQHandler(void) {

	if ((EXTI->PR1 & EXTI_PR1_PIF12) != 0) { // check if interrupt actually occured

    joystickSW = 1;
		// Clear interrupt pending bit
		EXTI->PR1 |= EXTI_PR1_PIF12;
	}
  delay(5);
}
