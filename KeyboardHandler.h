#ifndef KEYBOARD_HANDLER_H
#define KEYBOARD_HANDLER_H

#include <Arduino.h>

class KeyboardHandler {
  public:
  virtual uint8_t getKey();
  virtual uint8_t translateKey(uint8_t orig_key);
  virtual bool init();
};

#endif
