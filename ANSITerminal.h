#ifndef ASNSI_TERMINAL_h
#define ANSI_TERMINAL_H

#include <Arduino.h>
#include <WiFiClient.h>
#include "KeyboardHandler.h"
#include "ScreenRenderer.h"
#include "DebugUtil.h"



class ANSITerminal {
  public:

  struct cursorPos {
    uint8_t line;
    uint8_t column;
  };

  enum MoveCursorDir {
    UP,
    DOWN,
    LEFT,
    RIGHT
  };

  enum eraseType {
    END_OF_LINE,
    BEGINNING_OF_LINE,
    ENTIRE_LINE,
    END_OF_SCREEN,
    BEGINNING_OF_SCREEN,
    ENTIRE_SCREEN,
  };


  enum VK_KEYS {
    VK_ERASE = 7,
    VK_BACKSPACE = 8,
    VK_TAB = 9,
    VK_LF = 10,
    VK_CR = 13,
    VK_ESCAPE = 27,
    VK_SPACE = 32,
    VK_DELETE = 127,
  };  

  ANSITerminal(KeyboardHandler* keyboard, ScreenRenderer* screen, WiFiClient* client,
    DebugUtil* logger);
  bool init(uint16_t rows, uint16_t cols); 
  void loop();

  private:
  DebugUtil* logger;
  WiFiClient* netClient;
  KeyboardHandler* keyboard;
  ScreenRenderer* screen;

  const int net_retries = 3;

  uint16_t num_cols = 0;
  uint16_t num_rows = 0;

  // 0-19   off screen
  // 20-39  on screen
  // 50-59  off screen
  uint16_t first_display_row = 0;
  uint16_t last_display_row = 0;




  cursorPos saved_cursor_pos;

  

  #define TAB_SPACES 4

  //const String ESC_TERMINAL_CHARS = "ABCDEFGHIJKLMNOPQRSTUV";
  const String ESC_ERASE_TERMINAL_CHARS = "JK";
  const String ESC_COLOR_TERMINAL_CHARS = "m";
  const String ESC_CURSOR_TERMINAL_CHARS = "ABCDEHMfn78hl";

  const String NUMBER_CHARACTERS = "0123456789";

  #define ESC_FINAL_BYTE_MIN 0x40 
  #define ESC_FINAL_BYTE_MAX 0x7E
  #define MAX_ESC_SEQ_LEN 32 // For safety




  int current_text_color = TermColor::ESC_WHITE;
  int back_color = TermColor::ESC_BLACK;

  const uint16_t COLOR_MASK = 0b1111111100000000;
  const uint16_t CHAR_MASK =  0b0000000011111111;




  // + 1 for null character
  char esc_sequence[MAX_ESC_SEQ_LEN + 1]; 

  #define MAX_GET_NEXT_KEY_ATTEMPTS 5
  #define GET_NEXT_KEY_DELAY 100
  uint8_t key_read = 0;
  uint8_t byte_read = 0;

  uint16_t current_row = first_display_row;
  uint16_t current_col = 0;

  uint8_t last_key = 0;

// Methods.
  int16_t combineCharAndColor(char c, uint8_t color);
  void initEscSequence();
  bool isESCFinalByte(uint8_t key);
  void handleEscapeSequence(); 
  bool isCursorCommand(String escSequence); 
  char getCharFromInt(uint16_t val);
  uint8_t getColorFromInt(uint16_t val);
  void initRow(uint8_t row) ;
  void initDisplayBuffer();
  void disposeDisplayBuffer();
  uint8_t getDisplayRow(uint16_t buffer_row);
  void sendByte(uint8_t byte);
  uint8_t getByte();
  uint8_t getNextByte();
  bool getKeyboardInput(uint8_t& key);
  void saveCursor();
  void restoreCursor();
  void printCurrentChar();
  void copyRow(uint8_t srcRow, uint8_t destRow);
  void shiftRowsUp();
  void handleScroll();
  void handlePrintingChar(uint8_t key);
  void handleReturnKey();
  void handleTabKey();
  void handleVkErase();
  void handleBackspace();
  void moveAbsolutePosition(uint8_t line, uint8_t column);
  void moveRelativePosition(MoveCursorDir direction, uint8_t amount);
  void clearScreen();
  void handleColorCommand(String sequence);
  cursorPos parseEscapeCursorValues(String escSequence);
  uint8_t safeParseNumber(String strVal);
  uint8_t parseEscNumber(String escSequence);
  void hideShowCursor(String escSequence);
  void handleCursorCommand(String escSequence);
  bool isColorCommand(String escSequence);
  bool isEraseCommand(String escSequence);
  void swap(uint8_t* val1, uint8_t* val2);
  void doErase(
      uint8_t start_line,
      uint8_t end_line,
      uint8_t start_col,
      uint8_t end_col
  );
  String getCursorDirDesc(MoveCursorDir type);
  String getEraseTypeDesc(eraseType etype) ;
  void processEraseCommand(eraseType erase_type);
  void handleEraseCommand(String escSequence);
  void processEscapeSequence(String escSequence);
  void handleControlChar(uint8_t key);
  bool isControlChar(uint8_t key);
  void handleKey(uint8_t key);
  void printLine(uint8_t  row);
  void printLines();
  void sendTermCap();

  uint16_t* disp_buffer;  
};
#endif
