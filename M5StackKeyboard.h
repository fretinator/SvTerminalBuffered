#ifndef MSTACK_KEYBOARD_H
#define MSTACK_KEYBOARD_H

#include <Wire.h>
#include "KeyboardHandler.h"
#include "DebugUtil.h"

#define M5_KEYBOARD_ADDRESS     0x5F

class M5StackKeyboard : public KeyboardHandler {
public:
  M5StackKeyboard(DebugUtil* logger);
  virtual uint8_t getKey();
	virtual uint8_t translateKey(uint8_t orig_key);
  virtual bool init();
private:
  DebugUtil* logger;
};
	
#endif 

