#include "FS.h"
#include "SD.h"
#include "SPI.h"

// #define SD_MOSI 23
// #define SD_MISO 19
// #define SD_SCLK 18
// #define SD_CS 5

bool is_card_here = true;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    is_card_here = false;
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    is_card_here = false;
    return;
  }
  if(is_card_here) {
    Serial.println("Card here");
  } else {
    Serial.println("Card Error");
  }

}

void loop() {
  // put your main code here, to run repeatedly:

}
