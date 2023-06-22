
#include "stm32l476xx.h"
#include "LCD.h"
#include "SPI.h"
#include "SysTimer.h"

#include <stdio.h>

// initialize pins to output logical hi/lo to interface with LCD
void LCD_Control_Init(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; // Set GPIO Port A clock enable

		// Set to output mode for pins 0, 8, 11 (0b01)
		GPIOA->MODER &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE8 | GPIO_MODER_MODE11);
		GPIOA->MODER |= GPIO_MODER_MODE0_0 | GPIO_MODER_MODE8_0 | GPIO_MODER_MODE11_0;

		// Set Push-Pull output type (0b0)
    GPIOA->OTYPER &= ~(GPIO_OTYPER_OT0 | GPIO_OTYPER_OT8 | GPIO_OTYPER_OT11);

		// Set to veryfast output speed
    GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED0 | GPIO_OSPEEDR_OSPEED8 | GPIO_OSPEEDR_OSPEED11;

		// configure PUPDR
		GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD0 | GPIO_PUPDR_PUPD8 | GPIO_PUPDR_PUPD11);

    // properly initialize the LCD before use with reset
    GPIOA->ODR |= GPIO_ODR_OD0;
    delay(10);
    GPIOA->ODR &= ~GPIO_ODR_OD0;
    delay(10); // minor delay
    GPIOA->ODR |= GPIO_ODR_OD0;

    // change to command mode (0)
    GPIOA->ODR &= ~GPIO_ODR_OD11;

}

// set DC to command (0) or data (1) for data transfers
void LCD_DC(char flag) {
	if (flag) {
		GPIOA->ODR |= GPIO_ODR_OD11;
	} else {
		GPIOA->ODR &= ~GPIO_ODR_OD11;
	}
}

// reset LCD
void LCD_Reset() {
	GPIOA->ODR &= ~GPIO_ODR_OD0;
	delay(10); // minor delay
	GPIOA->ODR |= GPIO_ODR_OD0;
  delay(10); // minor delay
}

// flag : command (0) or data (1)
void LCD_Send_Byte(char flag, uint8_t byte) {
  LCD_DC(flag);
  SPI_Transfer_Byte(SPI1, byte);
}

// initializes registers of LCD
void LCD_Reg_Init() {

  GPIOA->ODR |= GPIO_ODR_OD8;
  delay(500);
  GPIOA->ODR &= ~GPIO_ODR_OD8;
  // PD (power down) off, horizontal addressing, extended innstruction (next send)
  LCD_Send_Byte(0, 0x21); // set operation (extended)
  LCD_Send_Byte(0, 0xD0); // set Vop

  // normal instruction
  LCD_Send_Byte(0, 0x20); // set operation (normal)
  LCD_Send_Byte(0, 0x0C); // display control set to normal mode
}

// clear the LCD
void LCD_Clear() {
  for (size_t i = 0; i < 504; i++) {
    LCD_Send_Byte(1, 0x00);
  }
}

// sets the current address of the LCD. Invalid ranges are corrected by a modulo
void LCD_Set_Address(uint8_t xPos, uint8_t yPos) {
  LCD_Send_Byte(0, 0x20); // set operation (normal)
  LCD_Send_Byte(0, (0x40 | (yPos % 6))); // change y address

  LCD_Send_Byte(0, 0x20); // set operation (normal)
  LCD_Send_Byte(0, (0x80 | (xPos % 84))); // change x address
}

// prints a character at the current address
void LCD_Print_Char(char c) {

  for (size_t i = 0; i < 5; i++) {
    if (ASCII[c - 0x20][i] != 0x00) {
      LCD_Send_Byte(1, ASCII[c - 0x20][i]);
    }
  }
  LCD_Send_Byte(1, 0x00);
}

// prints a string of characters
void LCD_Print_String(char* string) {
  while(*string) {
    if (*string != ' ') {
      LCD_Print_Char(*string);
    } else {
      LCD_Send_Byte(1, 0x00);
      LCD_Send_Byte(1, 0x00);
    }
    string++;
  }
}

// prints a card image
// we must account for the new address this might take, so we have to track the
// addresses before hand. The pixel width of this card will be 11 pixels wide
// max amount of cards across an LED screen ROW will be 7 which should be enough
// NOTE, THERE SHOULD BE ENOUGH ROOM FOR THE CARD, 2 Y ADDRESSES HEIGHT
void LCD_Print_Card(uint8_t xPos, uint8_t yPos, uint8_t suite, uint8_t value) {

  LCD_Set_Address(xPos, yPos);
  // first print upper left of card with the suite
  for (size_t i = 0; i < 5; i++) {
    if (i >= 2) {
      LCD_Send_Byte(1, (CARD_LINING_L[0][i] | SUITE[suite][i-2]));
    } else {
      LCD_Send_Byte(1, CARD_LINING_L[0][i]);
    }
  }

  // next print upper right of card and the value
  for (size_t i = 0; i < 6; i++) {
    if ( (i != 0) && (i <= 3) ) {
      LCD_Send_Byte(1, (CARD_LINING_R[0][i] | VALS[value][i-1]));
    } else {
      LCD_Send_Byte(1, CARD_LINING_R[0][i]);
    }
  }

  // change to the next row
  LCD_Set_Address(xPos, yPos + 1);

  // print the bottom parts
  for (size_t i = 0; i < 5; i++) {
    LCD_Send_Byte(1, CARD_LINING_L[1][i]);
  }
  for (size_t i = 0; i < 6; i++) {
    LCD_Send_Byte(1, CARD_LINING_R[1][i]);
  }
}

// clears from x1 to x2 and y1 to y2
// x1 should be less than x2 and y1 should be less than y2
// they should also follow the contraints of the LCD screen
void LCD_Clear_Block(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {

  for (size_t Y = y1; Y <= y2 ; Y++) {
    LCD_Set_Address(x1, Y);
    for (size_t X = x1; X <= x2; X++) {
      LCD_Send_Byte(1, 0x00);
    }
  }
}
