#ifndef ILI9341_SPI_RENDERER_H
#define ILI9341_SPI_RENDERER_H

#include <Arduino.h>
#include "ScreenRenderer.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "DebugUtil.h"


#ifdef ESP8266
   #define STMPE_CS 16
   #define TFT_CS   0
   #define TFT_DC   15
   #define SD_CS    2
#elif defined(ESP32) && !defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
   #define STMPE_CS 32
   #define TFT_CS   15
   #define TFT_DC   33
   #define SD_CS    14
#elif defined(TEENSYDUINO)
   #define TFT_DC   10
   #define TFT_CS   4
   #define STMPE_CS 3
   #define SD_CS    8
#elif defined(ARDUINO_STM32_FEATHER)
   #define TFT_DC   PB4
   #define TFT_CS   PA15
   #define STMPE_CS PC7
   #define SD_CS    PC5
#elif defined(ARDUINO_NRF52832_FEATHER)  /* BSP 0.6.5 and higher! */
   #define TFT_DC   11
   #define TFT_CS   31
   #define STMPE_CS 30
   #define SD_CS    27
#elif defined(ARDUINO_MAX32620FTHR) || defined(ARDUINO_MAX32630FTHR)
   #define TFT_DC   P5_4
   #define TFT_CS   P5_3
   #define STMPE_CS P3_3
   #define SD_CS    P3_2
#else
    // Anything else, defaults!
   #define STMPE_CS 6
   #define TFT_CS   9
   #define TFT_DC   10
   #define SD_CS    5
#endif

#define USE_ADAFRUIT_SHIELD_PINOUT

// PIXELS
#define SCREEN_INDENT 2
#define LINE_PAD 4
#define COL_PAD 2  
//#define BOARD_TFT_BACKLIGHT 42
#define USE_ADAFRUIT_SHIELD_PINOUT

class ILI9341SpiRenderer : public ScreenRenderer {
  public:
  ILI9341SpiRenderer(uint8_t textHeight, TermColor backcolor, TermColor forecolor,
    DebugUtil* logger);
  
  virtual void printCharacter(char c, uint16_t row, uint16_t col);
  virtual void printString(String str, uint16_t row, uint16_t col);
  virtual void clearScreen();
  virtual void setBackColor(TermColor color);
  virtual void setTextColor(TermColor color);
  virtual void fillscreen();
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
  
  private:
  DebugUtil* logger;
  
  // calculate later
  int16_t screen_w = 0;
  int16_t screen_h = 0;
  int text_height = 0;
  int text_width = 0; 
  uint16_t rows = 0;
  uint16_t cols = 0; 

  uint16_t yOffset = 0;

  uint8_t text_size = 1;

  Adafruit_ILI9341 screen = Adafruit_ILI9341(TFT_CS, TFT_DC);

  uint16_t getRowY(uint16_t row);
  // Characters are 0-based
  uint16_t getColX(uint16_t col);
  uint16_t mapColor(TermColor color);
  int scroll_line();
  void setupScrollArea(uint16_t tfa, uint16_t bfa);
  void scrollAddress(uint16_t vsp);
};

#endif
