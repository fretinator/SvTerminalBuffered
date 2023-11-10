#ifndef SCREEN_RENDERER_H
#define SCREEN_RENDERER_H

#include <Arduino.h>

  enum TermColor {
    ESC_RESET = 0,
    ESC_BLACK = 30,
    ESC_RED = 31,
    ESC_GREEN = 32,
    ESC_YELLOW = 33,
    ESC_BLUE = 34,
    ESC_MAGENTA = 35,
    ESC_CYAN = 36,
    ESC_WHITE = 37,
  };

class ScreenRenderer {
  public:

  inline TermColor getTermColorFromInt(uint8_t color) {
    switch(color) {
    case 0:
      return ESC_RESET;
    case 30:
      return ESC_BLACK;
    case 31:
      return ESC_RED;
    case 32:
      return ESC_GREEN;
    case 33:
      return ESC_YELLOW;
    case 34:
      return ESC_BLUE;
    case 35:
      return ESC_MAGENTA;
    case 36:
      return ESC_CYAN;
    case 37:
      return ESC_WHITE;
    default:
      return ESC_WHITE;
    }
  }

  inline uint8_t getIntFromColor(TermColor color) {
    switch(color) {
    case ESC_RESET:
        return 0;
      case ESC_BLACK:
        return 30;
      case ESC_RED:
        return 31;
      case ESC_GREEN:
        return 32;
      case ESC_YELLOW:
        return 33;
      case ESC_BLUE:
        return 34;
      case ESC_MAGENTA:
        return 35;
      case ESC_CYAN:
        return 36;
      case ESC_WHITE:
        return 37;
      default:
        return 37;
    }    
  }

  const static TermColor ESC_DEFAULT = ESC_WHITE;
  TermColor backcolor = ESC_BLACK;
  TermColor forecolor = ESC_WHITE;
  uint16_t rows;
  uint16_t cols;



  virtual void printCharacter(char c, uint16_t row, uint16_t col);
  virtual void printString(String str, uint16_t row, uint16_t col);
  virtual void clearScreen();
  virtual void setBackColor(TermColor color);
  virtual void setTextColor(TermColor color);
  virtual void fillScreen(uint8_t color);
  virtual void drawCursor(uint16_t row, uint16_t col);
  virtual void eraseCursor(uint16_t row, uint16_t col);
  virtual uint16_t getHeight();
  virtual uint16_t getWidth();
  virtual uint8_t getTextHeight();
  virtual uint8_t getTextWidth();
  virtual uint16_t getRows();
  virtual uint16_t getCols();
  virtual bool init();
  virtual bool scroll();
  

};
#endif
