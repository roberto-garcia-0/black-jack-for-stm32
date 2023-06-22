void BJ_Flash(uint8_t y, char* string);
void Initial_Screen(void);
void Initiate_Balance(void);
void Place_Bets(void);

void Deal_Cards(void);
uint8_t Player_1st_Choice(void);
uint8_t Player_Choice(void);
void Player_Hit(void);
void Player_Double(void);
uint8_t Player_Turn(void);
void Dealer_Turn(uint8_t playerResult);
void Dealer_Hit(void);
void Play_Black_Jack(void);

struct card {
  uint8_t suite;
  uint8_t value;
  struct card *next;
};

struct card* Deal_Card(void);
char Search_Card(struct card* find);
void Animate_Card(uint8_t cardPosX, uint8_t cardPosY, struct card* c);
void Free_Cards(struct card* c);
void Free_All_Cards(void);
void Count_Cards(struct card* start, uint8_t* hasAce, uint8_t* cardCount);
