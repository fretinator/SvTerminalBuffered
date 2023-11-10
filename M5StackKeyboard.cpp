#include "M5StackKeyboard.h"

M5StackKeyboard::M5StackKeyboard(DebugUtil* logger) {
  this->logger = logger;
}

uint8_t M5StackKeyboard::getKey() {
	uint8_t ret = 0;

  Wire.requestFrom(M5_KEYBOARD_ADDRESS, 1);
  
  if(Wire.available()) {
    ret = Wire.read(); 
  }

  return ret;
}
uint8_t M5StackKeyboard::translateKey(uint8_t orig_key) {
  return orig_key;
}

bool M5StackKeyboard::init() {
  logger->debug("Initializing Keyboard...");
  //pinMode(BOARD_POWERON, OUTPUT);
  //digitalWrite(BOARD_POWERON, HIGH);

  // Let warm up
  delay(500);

  Wire.begin();
  
  logger->debug("Keyboard initialized");
  return true;
}

