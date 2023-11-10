  #include <Arduino.h>
  #include "KeyboardHandler.h"
  
uint8_t KeyboardHandler::getKey() {
  return 0;
}
uint8_t KeyboardHandler::translateKey(uint8_t orig_key) {
	return orig_key;
}
bool KeyboardHandler::init() {
  return false;
}
