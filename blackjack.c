#include "stm32l476xx.h"
#include "blackjack.h"
#include "LCD.h"
#include "SysTimer.h"
#include "joystick.h"
#include "RNG.h"
#include "UART.h"
#include <stdio.h>
#include <stdlib.h>
// linked list structure for player and dealer
// struct card {
//   uint8_t suite;
//   uint8_t value;
//   struct card *next;
// };

struct card *playerHand;
struct card *playerRecentCard;
uint8_t playerCount;
uint8_t playerHasAce;
uint8_t playerCardX;

static uint32_t playerBet;
static uint32_t playerBalance;

struct card *dealerHand;
struct card *dealerRecentCard;
uint8_t dealerCount;
uint8_t dealerHasAce;
uint8_t dealerCardX;



// flashes text on screen until button is pressed
void BJ_Flash(uint8_t y, char* string) {
  while (!JoystickSW_Get()) {
    LCD_Set_Address(0, y);
    delay(250);
    LCD_Print_String(string);
    delay(250);
    LCD_Clear_Block(0, y, 83, y);
  }
}

void Initial_Screen() {
  LCD_Set_Address(0, 0);
  LCD_Print_String("Welcome");
  delay(500);

  LCD_Set_Address(0, 1);
  LCD_Print_String("To");
  delay(500);

  LCD_Set_Address(0, 2);
  LCD_Print_String("Black Jack");
  delay(500);

  BJ_Flash(3, "Press Button!");
  LCD_Clear();

}

// initialization of balance when game starts or player balance is not enough for
// a bet
void Initiate_Balance() {
  LCD_Set_Address(0, 0);
  LCD_Print_String("Enter");
  delay(500);
  LCD_Set_Address(0, 1);
  LCD_Print_String("Balance!");

  printf("Enter number of digits of balance!\n");
  char size = USART_Read_Str_to_Int(1);
  printf("Enter balance with %d digits!\n", size);
  playerBalance = USART_Read_Str_to_Int(size);

  LCD_Clear();
  LCD_Set_Address(0, 0);
  LCD_Print_String("You Entered:");
  char entered[size];
  sprintf(entered, "%d", playerBalance);

  LCD_Set_Address(0, 1);
  LCD_Print_Char('$');
  LCD_Print_String(entered);

  BJ_Flash(2, "Press Button!");
  LCD_Clear();

}

// betting phase of game
void Place_Bets() {
  LCD_Set_Address(0, 0);
  LCD_Print_String("Enter");
  delay(500);
  LCD_Set_Address(0, 1);
  LCD_Print_String("Bet!");
  delay(500);
  LCD_Set_Address(0, 2);
  LCD_Print_String("Balance:");
  char bal[10];
  sprintf(bal, "%d", playerBalance);
  LCD_Set_Address(0, 3);
  LCD_Print_Char('$');
  LCD_Print_String(bal);

  printf("Enter number of digits of bet!\n");
  char size = USART_Read_Str_to_Int(1);
  printf("Enter bet with %d digits!\n", size);
  playerBet = USART_Read_Str_to_Int(size);
  while (playerBet > playerBalance) {
    printf("Enter a bet less than $%d with %d digits!\n", playerBalance, size);
    playerBet = USART_Read_Str_to_Int(size);
  }
  playerBalance -= playerBet;

  LCD_Clear();
  LCD_Set_Address(0, 0);
  LCD_Print_String("You Entered:");
  char entered[size];
  sprintf(entered, "%d", playerBet);
  LCD_Set_Address(0, 1);
  LCD_Print_Char('$');
  LCD_Print_String(entered);

  BJ_Flash(2, "Press Button!");
  LCD_Clear();

  LCD_Set_Address(0, 0);
  LCD_Print_String("Your New");
  LCD_Set_Address(0, 1);
  LCD_Print_String("Balance:");
  // char entered[size];
  sprintf(entered, "%d", playerBalance);

  LCD_Set_Address(0, 2);
  LCD_Print_Char('$');
  LCD_Print_String(entered);

  BJ_Flash(3, "Press Button!");
  LCD_Clear();
}

// checks for duplicate cards in dealer and player hands
char Search_Card(struct card* find) {

  for (struct card* current = playerHand; current != NULL; current = current->next) {
    if ((find->suite == current->suite) && (find->value == current->value)) {
      return 1;
    }
  }
  for (struct card* current = dealerHand; current != NULL; current = current->next) {
    if ((find->suite == current->suite) && (find->value == current->value)) {
      return 1;
    }
  }
  return 0;
}

// creates unique, hardware randomized cards
struct card* Deal_Card() {
  struct card *res = (struct card*) malloc(sizeof(struct card));
  do {
    res->suite = RNG_Get_Val() % 4;
    res->value = RNG_Get_Val() % 13 + 1;
  } while(Search_Card(res));

  res->next = NULL;
  return res;
}

// creates basic animation of dealing card on screen
void Animate_Card(uint8_t cardPosX, uint8_t cardPosY, struct card* c) {
  for (int i = 72; i >= cardPosX; i -= 2) {
    LCD_Print_Card(i, cardPosY, c->suite, c->value);
    delay(10);
    if (i != cardPosX) {
      LCD_Clear_Block(cardPosX, cardPosY, 83, cardPosY + 1);
    }
  }
}

// counts cards from a starting reference card. Value is used to determine
// wins/losses/draws. This counts the best case scenario count regarding aces
void Count_Cards(struct card* start, uint8_t* hasAce, uint8_t* cardCount) {

  for (struct card* current = start; current != NULL; current = current->next) {
    if (current->value == 1) {
      if (*hasAce) {
        (*cardCount)++; // already has ace, so add one to count (more than 1 11 cost ace will cause bust)
      } else {
        *hasAce = 1; // signifies that an ace has a value of 11
        *cardCount += 11;
        if (*cardCount > 21) {
          // if 11 value causes bust, make it a 1 instead
          *cardCount -= 10;
          *hasAce = 2; // make note that no current ace can convert to 1 from 11
        }
      }
    } else if (current->value > 10) { // these are J, Q, K which should count as 10
      *cardCount += 10;
    } else { // count the rest of the cards as is
      *cardCount += current->value;
    }

    if ((*cardCount > 21) && (*hasAce == 1)){
      // if an ace of 11 value causes bust, make it a 1 instead
      *cardCount -= 10;
      *hasAce = 2; // make note of it that no current ace can convert to 1 from 11
    }
  }
}

// Initial dealing of cards
void Deal_Cards() {
  char str[3];
  playerCount = 0;
  dealerCount = 0;
  playerHasAce = 0;
  dealerHasAce = 0;
  playerCardX = 0;
  dealerCardX = 0;

  playerHand = Deal_Card();
  playerHand->next = Deal_Card();
  playerRecentCard = playerHand->next;
  dealerHand = Deal_Card();
  dealerHand->next = Deal_Card();
  dealerRecentCard = dealerHand->next;

  Count_Cards(playerHand, &playerHasAce, &playerCount);
  Count_Cards(dealerHand, &dealerHasAce, &dealerCount);

  Animate_Card(playerCardX, 4, playerHand);
  playerCardX += 12;
  Animate_Card(dealerCardX, 0, dealerHand);
  dealerCardX += 12;
  Animate_Card(playerCardX, 4, playerHand->next);
  playerCardX += 12;
  sprintf(str, "%d", playerCount);
  LCD_Set_Address(playerCardX, 5);
  LCD_Print_String(str);


  // // debug code
  // BJ_Flash(2, "Press Button!");
  // Player_Hit();
  // BJ_Flash(2, "Press Button!");
  // Player_Hit();
  // BJ_Flash(2, "Press Button!");
  // Animate_Card(12, 0, dealerHand->next);
  // sprintf(str, "%d", dealerCount);
  // LCD_Set_Address(24, 1);
  // LCD_Print_String(str);
  // Free_All_Cards();
}

// frees card linked list from memory
void Free_Cards(struct card* c) {
  if (c == NULL) {
    return;
  } else {
    Free_Cards(c->next);
    free(c);
  }
}

// frees dealer and player cards, also sets pointers to NULL
void Free_All_Cards() {
  Free_Cards(playerHand);
  Free_Cards(dealerHand);
  playerHand = NULL;
  dealerHand = NULL;
  playerRecentCard = NULL;
  dealerRecentCard = NULL;
}

enum playerChoice {
  HIT,
  STAND,
  DOUBLE,
  SURRENDER
};

// Handles polling of joystick and joystickSW interrupt
// Options show at y = 2 of LCD, returns choice
// This function accounts for when the player can double down
uint8_t Player_1st_Choice() {
  int choice = HIT;
  uint8_t prev = HIT;
  uint8_t d = 0;
  while (!JoystickSW_Get()) {
    switch (choice) {
      case HIT:
        LCD_Set_Address(0, 2);
        LCD_Print_String("HIT!");
        break;
      case STAND:
        LCD_Set_Address(0, 2);
        LCD_Print_String("STAND!");
        break;
      case DOUBLE:
        if (playerBalance < playerBet) {
          d = 1;
        } else {
          LCD_Set_Address(0, 2);
          LCD_Print_String("DOUBLE!");
        }
        break;
      case SURRENDER:
        LCD_Set_Address(0, 2);
        LCD_Print_String("SURRENDER!");
        break;
    }

    if (!d) {
      prev = choice;
      switch (Joystick_Poll_Reading()) {
        case 0:
          choice = (choice + 3) % 4;
          break;
        case 1:
          choice = (choice + 1) % 4;
          break;
        case 2:
          break;
      }
      delay(225);
      LCD_Clear_Block(0, 2, 83, 2);
    } else {
      d = 0;
      if (prev == STAND) {
        choice = SURRENDER;
      } else {
        choice = STAND;
      }
    }
  }
  return choice;
}

// use this after hitting once (hitting and standing only allowed)
uint8_t Player_Choice() {
  uint8_t choice = HIT;
  while (!JoystickSW_Get()) {
    switch (choice) {
      case HIT:
        LCD_Set_Address(0, 2);
        LCD_Print_String("HIT!");
        break;
      case STAND:
        LCD_Set_Address(0, 2);
        LCD_Print_String("STAND!");
        break;
    }



    switch (Joystick_Poll_Reading()) {
      case 0:
      case 1:
        choice = (choice + 1) % 2;
        break;
      case 2:
        break;
    }
    delay(225);
    LCD_Clear_Block(0, 2, 83, 2);
  }


  return choice;
}

// handles when player hits and animation and card count
void Player_Hit() {
  playerRecentCard->next = Deal_Card();
  playerRecentCard = playerRecentCard->next;
  Count_Cards(playerRecentCard, &playerHasAce, &playerCount);
  Animate_Card(playerCardX, 4, playerRecentCard);
  playerCardX += 12;

  char str[3];
  sprintf(str, "%d", playerCount);
  LCD_Set_Address(playerCardX, 5);
  LCD_Print_String(str);

}

void Player_Double() {
  playerBalance -= playerBet;
  playerBet += playerBet;
  Player_Hit();
}

// 0 - valid count; 1 - bust; 2 - surrender; 3 - BJ
uint8_t Player_Turn() {

  // restart balance if player doesnt have a valid balance
  if (playerBalance < 10) {
    Initiate_Balance();
  }

  // begin betting
  Place_Bets();

  // begin dealing cards
  Deal_Cards();

  // check for BJ's
  if (playerCount == 21) {
    BJ_Flash(2, "BLACK JACK!");
    return 3;
  } else {

    // player's actions
    uint8_t choice = Player_1st_Choice();
    // check if hit; if so use other set of choices afterwards
    if (choice == HIT) {
      Player_Hit();
      while((choice == HIT) && (playerCount <= 21)) {
        choice = Player_Choice();
        if (choice == HIT) {
          Player_Hit();
        }
      }
    } else if (choice == DOUBLE) {
      Player_Double();
    } else if (choice == SURRENDER) {
      return 2;
    }

    if (playerCount > 21) {
      return 1;
    }
  }
  return 0;

}

void Dealer_Hit() {
  dealerRecentCard->next = Deal_Card();
  dealerRecentCard = dealerRecentCard->next;
  Count_Cards(dealerRecentCard, &dealerHasAce, &dealerCount);
  Animate_Card(dealerCardX, 0, dealerRecentCard);
  dealerCardX += 12;

  char str[3];
  sprintf(str, "%d", dealerCount);
  LCD_Set_Address(dealerCardX, 1);
  LCD_Print_String(str);
}

void Dealer_Reveal() {
  char str[3];
  Animate_Card(12, 0, dealerHand->next);
  dealerCardX += 12;
  sprintf(str, "%d", dealerCount);
  LCD_Set_Address(dealerCardX, 1);
  LCD_Print_String(str);
}

void Dealer_Turn(uint8_t playerResult) {
  char str[10];
  Dealer_Reveal();
  if (playerResult == 0 || playerResult == 3) {
    while(dealerCount < 17) {
      delay(200);
      Dealer_Hit();
    }

    LCD_Set_Address(0, 3);
    if (playerCount > dealerCount || dealerCount > 21) {
      if (playerResult == 3) {
        playerBalance += 2.5*playerBet;
        sprintf(str, "%d", (int)(2.5*playerBet));
      }  else {
        playerBalance += 2*playerBet;
        sprintf(str, "%d", 2*playerBet);
      }
      LCD_Print_String("+ $");
      LCD_Print_String(str);
      BJ_Flash(2, "YOU WIN!");

    } else if (playerCount == dealerCount) {
      playerBalance += playerBet;
      sprintf(str, "%d", playerBet);
      LCD_Print_String("+ $");
      LCD_Print_String(str);
      BJ_Flash(2, "PUSH!");

    } else {
      sprintf(str, "%d", playerBet);
      LCD_Print_String("- $");
      LCD_Print_String(str);
      BJ_Flash(2, "YOU LOSE!");
    }

  } else if (playerResult == 2) {
    LCD_Set_Address(0, 3);
    playerBalance += playerBet/2;
    sprintf(str, "%d", playerBet/2);
    LCD_Print_String("+ $");
    LCD_Print_String(str);
    BJ_Flash(2, "SURRENDERED!");

  } else if (playerResult == 1) {
    LCD_Set_Address(0, 3);
    sprintf(str, "%d", playerBet);
    LCD_Print_String("- $");
    LCD_Print_String(str);
    BJ_Flash(2, "BUST!");
  }
  LCD_Clear();
}

void Play_Black_Jack(void) {
  Initial_Screen();
  Initiate_Balance();
  Free_All_Cards();
  while (1) {
    Dealer_Turn(Player_Turn());
    Free_All_Cards();
    // delay(750);
  }
}
