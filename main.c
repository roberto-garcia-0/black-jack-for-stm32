

#include "stm32l476xx.h"
#include "SPI.h"
#include "SysClock.h"
#include "SysTimer.h"
#include "ADC.h"
#include "LCD.h"
#include "UART.h"
#include "joystick.h"
#include "RNG.h"
#include "blackjack.h"

#include <stdio.h>

/*
-PIN assignments-
PA0   - RST -> GPO (active low)
PA1   - joystick VRy -> ADC
PA4   - CE -> GPO
PA5   - CLK -> SPI1_SCK
PA6   - N/A -> SPI1_MISO (No use in this case)
PA7   - DIN -> SPI1_MOSI
PA8   - LIGHT -> GPO
PA11  - DC -> GPO (command/data) (0/1)
PA12  - joystick SW -> Input (interrupt needed)

PB6   - USART1_TX
PB7   - USART1_RX
*/


int main(void){
	System_Clock_Init();  //HSI - 16MHz
	SysTick_Init();

  RNG_Init();

  ADC_Init();
  JoystickSW_EXTI_Init();

	SPI1_GPIO_Init();
	SPI1_Init();

  LCD_Control_Init();
  LCD_Reg_Init();
  LCD_Clear();

  UART1_Init();
  UART1_GPIO_Init();
  USART_Init(USART1);

  LCD_Set_Address(12, 2);
  LCD_Print_String("Hello World!");
  delay(1000);
  LCD_Clear();

	while(1) {

	Play_Black_Jack();



	}
}
