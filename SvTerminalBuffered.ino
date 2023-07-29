#include <WiFi.h>
#include <WiFiClient.h>
#include <M5Stack.h>
#include "secrets.h"

#define KEYBOARD_I2C_ADDR 0X08
#define KEYBOARD_INT      5
uint8_t key_read = 0;
uint8_t byte_read = 0;




#define NUM_COLS 50
#define NUM_ROWS 60

// 0-19   off screen
// 20-39  on screen
// 50-59  off screen
#define FIRST_DISP_LINE 20
#define LAST_DISP_LINE 39 

// PIXELS
#define LEFT_PAD 4 

struct cursorPos {
  uint8_t line;
  uint8_t column;
};

cursorPos saved_cursor_pos;

enum MOVE_CURSOR_DIR {
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

// calculate later
int16_t screen_w = 0;
int16_t screen_h = 0;
int text_height = 0;
int text_width = 0; 

enum VK_KEYS {
  VK_ERASE = 7,
  VK_BACKSPACE = 8,
  VK_TAB = 9,
  VK_LF = 10,
  VK_CR = 13,
  VK_ESCAPE = 27,
  VK_SPACE = 32,
  VK_DELETE = 127,
  VK_FACES_INS = 184, // Insert on Faces Keyboard
  VK_FACES_TAB = 186,
};

#define TAB_SPACES 4

//const String ESC_TERMINAL_CHARS = "ABCDEFGHIJKLMNOPQRSTUV";
const String ESC_ERASE_TERMINAL_CHARS = "JK";
const String ESC_COLOR_TERMINAL_CHARS = "m";
const String ESC_CURSOR_TERMINAL_CHARS = "ABCDEHMfn78hl";

const String NUMBER_CHARACTERS = "0123456789";

#define ESC_FINAL_BYTE_MIN 0x40 
#define ESC_FINAL_BYTE_MAX 0x7E
#define MAX_ESC_SEQ_LEN 32 // For safety


const uint8_t  ESC_RESET = 0;
const uint8_t  ESC_BLACK = 30;
const uint8_t  ESC_RED = 31;
const uint8_t  ESC_GREEN = 32;
const uint8_t  ESC_YELLOW = 33;
const uint8_t  ESC_BLUE = 34;
const uint8_t  ESC_MAGENTA = 35;
const uint8_t  ESC_CYAN = 36;
const uint8_t  ESC_WHITE = 37;
const uint8_t  ESC_DEFAULT = ESC_WHITE;


uint8_t current_color = ESC_WHITE;

const uint16_t COLOR_MASK = 0b1111111100000000;
const uint16_t CHAR_MASK =  0b0000000011111111;




// + 1 for null character
char esc_sequence[MAX_ESC_SEQ_LEN + 1]; 

#define MAX_GET_NEXT_KEY_ATTEMPTS 5
#define GET_NEXT_KEY_DELAY 100

uint16_t disp_buffer[NUM_ROWS * NUM_COLS];


uint8_t current_row = FIRST_DISP_LINE;
uint8_t current_col = 0;
uint8_t last_key = 0;

WiFiClient netClient;
#define MAX_WIFI_ATTEPTS 20
const int net_retries = 3;



uint16_t combineCharAndColor( char c, uint8_t color) {
  return 256 * color + c;
}

char getCharFromInt(uint16_t val) {
  return char(val & CHAR_MASK);
}

uint8_t getColorFromInt(uint16_t val) {
  return (val & COLOR_MASK) / 256;
}


void initRow(uint8_t row) {
  if(row >= NUM_ROWS) {
    Serial.print("**** Attempt to init row beyond end of array, ROW = : ");
    Serial.println(row);
  } else {
    for(uint8_t col = 0; col < NUM_COLS; col++) {
      uint16_t pos = (row * NUM_COLS) + col;

      disp_buffer[pos] = combineCharAndColor(' ', ESC_DEFAULT);
    } 
  }
}

void initDispBuffer() {
  for(uint8_t row = 0; row < NUM_ROWS; row++) {
    initRow(row);
  }
}

void initEscSequence() {
  for(int x = 0; 0 < MAX_ESC_SEQ_LEN;x++) {
    esc_sequence[x] = '\0';
  }
  esc_sequence[MAX_ESC_SEQ_LEN] = '\0';
}


bool setupWiFi() {
  uint8_t curAttempts = 0;
   
  WiFi.begin(my_ssid, my_password);
  
  while (WiFi.status() != WL_CONNECTED &&
      curAttempts < MAX_WIFI_ATTEPTS) {
    Serial.print(WiFi.status());
    delay(500);
    curAttempts++;
    //Serial.print(".");
    }

  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed.");
    return false;
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); 
  return true; 
}

bool connectToHost() {
  IPAddress addr;
  addr.fromString(host);
  return netClient.connect(addr, port);
}



void sendByte(uint8_t byte) {
  netClient.write(byte);
}

uint8_t getByte() {
  if(netClient.available()) {
    return uint8_t(netClient.read());
  }

  return 0;
}

void setup() {
  Serial.begin(115200);

  M5.begin();
  M5.Power.begin();
  // correctly
  M5.Lcd.fillScreen(TFT_BLACK);

  if(!setupWiFi()) {
    M5.lcd.println("Unable to connect to WiFi");
    delay(100);
    while(true);
  }

  if(!connectToHost()) {
    M5.lcd.println("Unable to connect to Host");
    delay(100);
    while(true);
  }

  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.drawCentreString("Net terminal ", 320 / 2, 0, 2);


  delay(3000);

  M5.lcd.clear();
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  screen_w = M5.lcd.width();
  screen_h = M5.lcd.height();

  text_width = M5.lcd.textWidth("W");
  //text_width -= 2;
  Serial.print("Text Width: ");
  Serial.println(text_width);

  text_height = screen_h / (LAST_DISP_LINE - FIRST_DISP_LINE + 1);

  // Change colour for scrolling zone text
  initDispBuffer();
  drawCursor();
}


bool getKeyboardInput(uint8_t& key) {
  if (digitalRead(KEYBOARD_INT) == LOW) {
    Wire.requestFrom(KEYBOARD_I2C_ADDR, 1);  // request 1 byte from keyboard
    if (Wire.available()) {
        uint8_t key_val = Wire.read();  // receive a byte as character
        //Serial.print("Key read: ");
        //+Serial.println(key_val);
        if(key_val > 0) {
          key = key_val;
          return true;
        }       
    }
  }

  return false;
}

// Will try to get next key for sequence with retries
uint8_t getNextByte() {
  uint8_t cur_attempts = 0;
  bool byte_retrieved = false;
  uint8_t the_byte = 0;

  while(!byte_retrieved && cur_attempts < MAX_GET_NEXT_KEY_ATTEMPTS) {
    cur_attempts++;
    the_byte = getByte();

    byte_retrieved = the_byte > 0;

    if(!the_byte) {
      delay(GET_NEXT_KEY_DELAY);
    }
  }
  
  return the_byte;
}

uint16_t getCurrentRowY() {
  return (current_row - FIRST_DISP_LINE) * text_height;
}

// Characters are 0-based
uint8_t getCurrentCharX() {
  return LEFT_PAD + (current_col * text_width);
}


void saveCursor() {
  saved_cursor_pos.line = current_row - FIRST_DISP_LINE;
  saved_cursor_pos.column = current_col;
}

void restoreCursor() {
  eraseCursor();

  current_row = saved_cursor_pos.line + FIRST_DISP_LINE;
  current_col = saved_cursor_pos.column;

  drawCursor();
}


void drawCursor() {
  uint8_t y = getCurrentRowY();
  uint8_t x = getCurrentCharX();
  M5.lcd.drawRect(x, y, text_width, text_height, TFT_BLUE);
}

void eraseCursor() {
  uint8_t y = getCurrentRowY();
  uint8_t x = getCurrentCharX();
  M5.lcd.drawRect(x, y, text_width, text_height, TFT_BLACK);
}



void printCurrentChar() {
  uint16_t int_val = disp_buffer[current_row * NUM_COLS + current_col];
  char the_char = getCharFromInt(int_val);
  uint8_t color = getColorFromInt(int_val);
  uint8_t ypos = getCurrentRowY();
  uint8_t xpos = getCurrentCharX();

  M5.lcd.fillRect(xpos, ypos, text_width, text_height, TFT_BLACK);
  M5.lcd.setCursor(xpos, ypos);
  
  if(current_color != color) {
    setTextColor(color);
    current_color = color;
  }
  
  M5.lcd.print(the_char);
}

void copyRow(uint8_t srcRow, uint8_t destRow) {
  Serial.println("");
  for(uint8_t col = 0; col < NUM_COLS;col++) {
    disp_buffer[(destRow * NUM_COLS) + col] = disp_buffer[(srcRow * NUM_COLS) + col]; 
  }
}

/*
  Shift contents of all rows up 1. 
  Row 0 will be replaced by 1, 1 by 2, etc
  Last rows will be initialized with spaces.
*/
void shiftRowsUp() {
  Serial.println("Shifting rows up");
  for(uint8_t row = 1; row < NUM_ROWS; row++) {
    copyRow(row, row-1);
  }
  initRow(NUM_ROWS - 1);
}

// Copy rows 1-NUM_ROWS - 1 to
// to rows 0 - NUM_ROWS - 2  
void handleScroll() {
  eraseCursor();
  shiftRowsUp();
  printLines();
}


void debugPrint(uint8_t key) {
  //Serial.println("Printing character:");
  //Serial.print("\tByte Value: ");
  //Serial.println(key);
  //Serial.print("\tChar Value:");
  //Serial.println(char(key));
}

void handlePrintingChar(uint8_t key) {
  debugPrint(key);
  eraseCursor();
  if(current_col > NUM_COLS - 1) {
    current_row++;
    current_col = 0;

    if(current_row > LAST_DISP_LINE) {
      handleScroll();
      current_row = LAST_DISP_LINE;
    } 
  }

  disp_buffer[current_row * NUM_COLS + current_col] = combineCharAndColor(char(key), current_color);
  printCurrentChar();
  current_col++;
  drawCursor();
}

void handleReturnKey() {
  eraseCursor();
  current_row++;

  current_col = 0;

  if(current_row > LAST_DISP_LINE) {
    current_row = LAST_DISP_LINE;
    handleScroll();
  }  else {
    //printCurrentChar();
  }
  drawCursor();
}

void handleTabKey() {
  for(int x = 0; x < TAB_SPACES; x++) {
    handleKey(VK_SPACE);
  }
}

void handleVkErase() {
  eraseCursor();
  disp_buffer[current_row * NUM_COLS + current_col] = ' ';
  printCurrentChar();
  drawCursor();  
}

void handleBackspace() {
  if(current_col > 0) {
    eraseCursor();
    current_col--;
    //do not erase
    //disp_buffer[current_row * NUM_COLS + current_col] = ' ';
    printCurrentChar();
    drawCursor();
  }
}




// Line is 
void moveAbsolutePosition(uint8_t line, uint8_t column) {
  Serial.println("\tMove Absolute: " + String(line) + "," + String(column));
  eraseCursor();

  current_row = line;
  
  if(column > (NUM_COLS - 1)) {
    current_col = NUM_COLS - 1;
  } else {
    if(column < 0) {
      current_col = 0;
    } else {
      current_col = column;
    }
  }

  drawCursor();
}


void moveRelativePosition(MOVE_CURSOR_DIR direction, uint8_t amount) {
  Serial.println("\t handling Move relative: " + getCursorDirDesc(direction))
    + " - " + String(amount);
  switch(direction) {
    case UP:
      moveAbsolutePosition(current_row - amount, current_col);
      break;
    case DOWN:
      moveAbsolutePosition(current_row  + amount, current_col);
      break;
    case LEFT:
      moveAbsolutePosition(current_row, current_col - amount);
      break;
    case RIGHT:
      moveAbsolutePosition(current_row, current_col + amount);
      break;
  }
  Serial.println("\tCompleted move relative.");
}

bool isESCFinalByte(uint8_t key) {
  return (key >= uint8_t('A') and key <= uint8_t('Z'))
    || (key >= uint8_t('a') and key <= uint8_t('z'));
}

void handleEscapeSequence() {
  Serial.println("\t[ESC} Entering Escape Sequence");
  uint8_t the_byte = 0;
  bool endSeqFound = false;
  uint8_t seq_bytes_read = 0;
  bool continueReading = true;

  // read until we read final byte character
  while(continueReading && seq_bytes_read < MAX_ESC_SEQ_LEN &&
    (the_byte = getNextByte()) > 0 ){
      esc_sequence[seq_bytes_read] = char(the_byte);
      seq_bytes_read++;
      continueReading = !isESCFinalByte(the_byte);
  }

  esc_sequence[seq_bytes_read] = '\0';

  Serial.print("\tEscape sequence: ");
  Serial.print("ESC");
  Serial.println(esc_sequence);
  processEscapeSequence(esc_sequence);
}

void clearScreen() {
  initDispBuffer();
  
  current_row = FIRST_DISP_LINE;
  current_col = 0;
  
  m5.lcd.clear();
  drawCursor();
}

uint8_t setTextColor(uint8_t color) {
  switch(color) {
      case ESC_RESET:
        M5.lcd.setTextColor(TFT_WHITE);
        return ESC_RESET;
      case ESC_BLACK:
        M5.lcd.setTextColor(TFT_BLACK);
        return ESC_BLACK;
      case ESC_RED:
        M5.lcd.setTextColor(TFT_RED);
        return ESC_RED;
      case ESC_GREEN:
        M5.lcd.setTextColor(TFT_GREEN);
        return ESC_GREEN;
      case ESC_YELLOW:
        M5.lcd.setTextColor(TFT_YELLOW);
        return ESC_YELLOW;
      case ESC_BLUE:
        M5.lcd.setTextColor(TFT_BLUE);
        return ESC_BLUE;
      case ESC_MAGENTA:
        M5.lcd.setTextColor(TFT_MAGENTA);
        return ESC_MAGENTA;
      case ESC_CYAN:
        M5.lcd.setTextColor(TFT_CYAN);
        return ESC_CYAN;
      case ESC_WHITE:
        M5.lcd.setTextColor(TFT_WHITE);
        return ESC_WHITE;
      default:
         M5.lcd.setTextColor(TFT_WHITE);
        return ESC_WHITE;
    }
}

void handleColorCommand(String sequence) {
  Serial.print("***** Handling Color Command: " + sequence);
  int char_pos = sequence.indexOf(';');

  if(char_pos > 0) {
    // Should be something like 01;34m
    String my_color = sequence.substring(char_pos + 1, sequence.length() - 1);
    uint8_t esc_color = my_color.toInt();
    Serial.print(", COLOR = ");
    Serial.println(esc_color);
    
   
    current_color =  setTextColor(esc_color);
  } else {
    Serial.println(", RESET COLOR");
    M5.lcd.setTextColor(TFT_WHITE);
    current_color = ESC_DEFAULT;
  }
}


bool isCursorCommand(String escSequence) {
  char last_char = escSequence.charAt(escSequence.length() - 1);
  return ESC_CURSOR_TERMINAL_CHARS.indexOf(last_char) >= 0;
}

cursorPos parseEscapeCursorValues(String escSequence) {
  cursorPos ret;

  Serial.println("\t**** Parsing cursor command: " + escSequence);

  if(escSequence.length() <= 1 ) {
    ret.line = FIRST_DISP_LINE;
    ret.column = 0;
    Serial.println("\t**** Returning defaults.");
    return ret;
  } else {
    String strNums = escSequence;
    // We don't want '[' or termninating chacter like 'A'
    if(escSequence.charAt(0) == '[') {
      strNums = escSequence.substring(1, escSequence.length() - 1);
    } else {
      strNums = escSequence.substring(0, escSequence.length() - 1);
    }  

    Serial.println("\t**** Parsing cursor numbers: " + strNums);

    uint8_t pos = strNums.indexOf(';');
    if( pos < 0) {
      // Assume only 1 value provided.Default the the column to current
      ret.line = parseEscNumber(escSequence) - 1 + FIRST_DISP_LINE;
      // Row and column are 1-based with ansi escape sequence
      ret.column = current_col;
      Serial.print("\t**** Only line provided, line parsed = ");
      Serial.print(ret.line);
      Serial.print(", current col = ");
      Serial.println(ret.column);
      return ret; 
    } else {
      if(pos == 0) {
        // Wew skipped the line number, only column provided
        ret.line = current_row;
        ret.column = parseEscNumber(escSequence.substring(1, escSequence.length() - 1)) - 1;
        Serial.print("\t**** Only column provided, column parsed = ");
        Serial.print(ret.column);
        Serial.print(", current line = ");
        Serial.println(ret.line);
        return ret;
      } else {
        String strRow = strNums.substring(0, pos);
        String strCol = strNums.substring(pos + 1);

        Serial.print("\t**** Parsed row and column strings: row =");
        Serial.print(strRow);
        Serial.print(", column = ");
        Serial.println(strCol);

        ret.line = safeParseNumber(strRow) - 1 + FIRST_DISP_LINE;
        ret.column = safeParseNumber(strCol) - 1;

        Serial.println("\t**** Final actual cursor numbers, row = ");
        Serial.print(ret.line);
        Serial.print(", Column = ");
        Serial.println(ret.column);
        return ret;
      }
    }
  }
}

uint8_t safeParseNumber(String strVal) {
 int curChar = 0;
    while(curChar < strVal.length() &&
        NUMBER_CHARACTERS.indexOf(strVal.charAt(curChar)) >= 0) {
      curChar++;
    }

    if(curChar < strVal.length()) {
      strVal = strVal.substring(0, curChar);
    }

    uint8_t numVal = uint8_t(strVal.toInt());

    if(numVal < 1) {
      return 1;
    }

    return numVal;
}

uint8_t parseEscNumber(String escSequence) {
  String strNum = "";

  if(escSequence.length() <= 1) {
    return 1;
  } else {
    // We don't want '[' or termninating chacter like 'A'
    if(escSequence.charAt(0) == '[') {
      strNum = escSequence.substring(1, escSequence.length() - 2);
    } else {
      strNum = escSequence.substring(0, escSequence.length() - 2);
    }

   return safeParseNumber(strNum);
  }
}

void hideShowCursor(String escSequence) {
  if(escSequence.charAt(escSequence.length() - 1) == 'h') {
    drawCursor();
  } else {
    eraseCursor();
  }
}

/*
  "[pnA"    	// cursor up pn times - stop at top
  "[pnB"    	// cursor down pn times - stop at bottom
  "[pnC"    	// cursor right pn times - stop at far right
  "[pnD"    	// cursor left pn times - stop at far left
  "[pl;pcH" 	// set cursor position - pl Line, pc Column
  "[H" 		  // set cursor home
  "[pl;pcf"	// set cursor position - pl Line, pc Column
  "[f"		    // set cursor home
  "[s"	save cursor position (SCO)
  "[u"	restores the cursor to the last saved position (SCO)

  "M"		    // cursor up - at top of region, scroll down
  "E"		    // next line (same as CR LF)
  "7"		    // save cursor position(char attr,char set,org)
  "8"		      // restore position (char attr,char set,origin)
*/

void handleCursorCommand(String escSequence) {
  Serial.println("\tHandling Cursor command: " + escSequence);
  cursorPos cursor;
  switch(escSequence.charAt(escSequence.length() - 1)) {
    case 'A':
      moveRelativePosition(UP, parseEscNumber(escSequence));
      break;
    case 'M':
      moveRelativePosition(UP, parseEscNumber(escSequence));
      break;
    case 'B':
      moveRelativePosition(DOWN, parseEscNumber(escSequence));
      break;
    case 'C':
      moveRelativePosition(RIGHT, parseEscNumber(escSequence));
      break;
    case 'D':
      moveRelativePosition(DOWN, parseEscNumber(escSequence));
      break;
    case 'H':
      cursor = parseEscapeCursorValues(escSequence);
      moveAbsolutePosition(cursor.line, cursor.column);
      break;
    case 'f':
      cursor = parseEscapeCursorValues(escSequence);
      moveAbsolutePosition(cursor.line, cursor.column);
      break;
    case 'E':
      handleReturnKey();
      break;
    case 's':
      saveCursor();
      break;
    case '7':
      saveCursor();
      break;
    case 'u':
      restoreCursor();
      break;
    case '8':
      restoreCursor();
      break;
    case 'h':
      hideShowCursor(escSequence);
      break;
    case 'l':
      hideShowCursor(escSequence);
      break;
    default:
      Serial.println("Unahandled Cursor Command: " + escSequence);
      break;
  }

  Serial.println("\t completed cursor command.");
}


bool isColorCommand(String escSequence) {
  char last_char = escSequence.charAt(escSequence.length() - 1);
  return ESC_COLOR_TERMINAL_CHARS.indexOf(last_char) >= 0;
}


bool isEraseCommand(String escSequence) {
  char last_char = escSequence.charAt(escSequence.length() - 1);
  return ESC_ERASE_TERMINAL_CHARS.indexOf(last_char) >= 0;
}

uint8_t swap(uint8_t* val1, uint8_t* val2) {
  uint8_t tmp = *val1;
  *val1 = *val2;
  *val2 = tmp;
}


void doErase(
    uint8_t start_line,
    uint8_t end_line,
    uint8_t start_col,
    uint8_t end_col
 ) {
  Serial.print("Erasing from line ");
  Serial.print(start_line);
  Serial.print(" to line ");
  Serial.print(end_line);
  Serial.print(", col ");
  Serial.print(start_col);
  Serial.print(" to ");
  Serial.println(end_col);
  
  // Sanity checks
  if(end_line < start_line) {
    swap(&start_line, &end_line);
  }

  if(end_col < start_col) {
    swap(&start_col, &end_col);
  }  

  if(start_line < 0) {
    start_line = 0;
  }

  if(start_line > NUM_ROWS -1) {
    start_line = NUM_ROWS - 1;
  }

  if(end_col > NUM_COLS - 1) {
    end_col = NUM_COLS - 1;
  }
  uint8_t myLine = start_line;
  uint8_t myCol = start_col;

  for(uint8_t row = start_line; row <= end_line; row++) {
    for(uint8_t col = start_col; col <= end_col;col++) {
      disp_buffer[row * NUM_COLS + col] = combineCharAndColor(' ', ESC_DEFAULT);
    }
  }
}

String getCursorDirDesc(MOVE_CURSOR_DIR type) {
  switch(type) {
    case DOWN:
      return "Down";
    case UP:
      return "Up";
    case LEFT:
      return "Left";
    case RIGHT:
      return "Right";
  }
}

String getEraseTypeDesc(eraseType etype) {
  switch(etype) {
    case END_OF_LINE:
      return "EOL";
    case BEGINNING_OF_LINE:
      return "BOL";
    case END_OF_SCREEN:
      return "EOS";
    case BEGINNING_OF_SCREEN:
      return "BOS";
    case ENTIRE_SCREEN:
      return "ALL";
  }
}

void processEraseCommand(eraseType erase_type) {
  Serial.println("Erasing: " + getEraseTypeDesc(erase_type));
  eraseCursor();

  switch(erase_type) {
    case END_OF_LINE:
      doErase(current_row, 
        current_row, 
        current_col, 
        NUM_COLS);
      break;
    case BEGINNING_OF_LINE:
      doErase(current_row, 
        current_row, 
        0, 
        current_col);
      break;
    case END_OF_SCREEN:
      doErase(current_row, 
        current_row, 
        current_col, 
        NUM_COLS - 1);
      doErase(current_row + 1, 
        LAST_DISP_LINE, 
        0, 
        NUM_COLS - 1);
      break;
    case BEGINNING_OF_SCREEN:
      doErase(0 ,
        current_row - 1, 
        0, 
        NUM_COLS - 1);
      doErase(current_row , 
        current_row, 
        0, 
        current_col);
      break;
    case ENTIRE_SCREEN:
      doErase(FIRST_DISP_LINE,
        LAST_DISP_LINE, 
        0, 
        NUM_COLS - 1);
  }

  Serial.println("Completed erasing.");
  printLines();
  drawCursor();
}

// Erasing:
/*

  "[K"		    // erase to end of line (inclusive)
  "[0K"		  // erase to end of line (inclusive)
  "[1K"		  // erase to beginning of line (inclusive)
  "[2K"		  // erase entire line (cursor doesn't move)
  "[J"		    // erase to end of screen (inclusive)
  "[0J"		  // erase to end of screen (inclusive)
  "[1J"		  // erase to beginning of screen (inclusive)
  "[2J"		    // erase entire screen (cursor doesn't move)
};
*/

void handleEraseCommand(String escSequence) {
  Serial.println("\tHandling erase command: " + escSequence);
  switch(escSequence.charAt(escSequence.length() - 1)) {
    case 'J':
      if(escSequence.length() <= 2
        || escSequence.charAt(1) == '0') {
          processEraseCommand(END_OF_SCREEN);
      } else {
        if(escSequence.charAt(1) == '1') {
           processEraseCommand(BEGINNING_OF_SCREEN);
        } else {
          processEraseCommand(ENTIRE_SCREEN);
        }
      }
      break;
    case 'K':
      if(escSequence.length() <= 2
        || escSequence.charAt(1) == '0') {
          processEraseCommand(END_OF_LINE);
      } else {
        if(escSequence.charAt(1) == '1') {
           processEraseCommand(BEGINNING_OF_LINE);
        } else {
          processEraseCommand(ENTIRE_LINE);
        }
      }
      break;
    default:
      Serial.println("Unhandled erase esc sequence: " + escSequence);
      break;
  }

  Serial.println("\tDone erasing.");
}

void processEscapeSequence(String escSequence) {
  Serial.println("\tProcessing escSequence: " + escSequence);
  if(isColorCommand(escSequence)) {
    handleColorCommand(escSequence);
  } else {
    if(isCursorCommand(escSequence)) {
      handleCursorCommand(escSequence);
    } else {
      if(isEraseCommand(escSequence)) {
        handleEraseCommand(escSequence);
      } else {
        Serial.println("Unhandled Escape Sequnce: " + escSequence);
      }
    }
  }


  Serial.println("Esc Sequence completed: " + escSequence);
}

void handleControlChar(uint8_t key) {
  //("Handling control character: ");
  //Serial.print(key);
  switch(key) {
    case VK_ESCAPE:
      //Serial.println(" - Enter escape handler");
      handleEscapeSequence();
      break;
    case VK_ERASE:
      //Serial.println(" - Handled");
      handleVkErase();
      break;
    case VK_BACKSPACE:
      //Serial.println(" - Handled");
      handleBackspace();
      break;
    //case VK_CR:
    case VK_LF:
      //Serial.println(" - Handled");
      handleReturnKey();
      break;
    case VK_TAB:
      handleTabKey();
      break;
    case VK_FACES_TAB:
      handleTabKey();
      break;
    default:
      Serial.println(" - Unhandled");
  }
}

bool isControlChar(uint8_t key) {
  return key < 32 || VK_DELETE == key || VK_FACES_TAB == key || VK_TAB == key;
}

uint8_t translateKeyForSend(uint8_t key) {
  if(key == 183) {
    return 17;
  }

  // M5Stack Faces Keyboard has no esc
  // Map "END" to "ESC"
  if(key == VK_FACES_INS) {
    return VK_ESCAPE;
  }

  return key;
}

void handleKey(uint8_t key) {
  Serial.print(key);
  Serial.print(",");
  Serial.println(char(key));

  if(isControlChar(key)) {
    handleControlChar(key);
  } else {
    handlePrintingChar(key);
  }

  last_key = key;
}

void printLine(uint8_t  row) {
  int which_char = 0;
  uint16_t the_val;
  char c;
  uint8_t char_color;
  uint8_t last_color = ESC_WHITE;
  bool is_null_char = false;

  // Row 20 is the first row to display
  uint16_t lineY = (row - FIRST_DISP_LINE)  * text_height;
  
  while(which_char <  NUM_COLS) {
    the_val = disp_buffer[row * NUM_COLS + which_char];
    c = getCharFromInt(the_val);
    char_color = getColorFromInt(the_val);
    if(last_color != char_color) {
      setTextColor(char_color);
      last_color = char_color;

    }

    if('\0' == c) {
      is_null_char = true;
    } else {
      //Serial.printf("Print character %c at line %d - x = %d y = %d", 
       // c, row, which_char * text_width + LEFT_PAD, lineY);
      //Serial.println();
      M5.lcd.setCursor(which_char * text_width + LEFT_PAD, lineY);
      M5.Lcd.print(c);
      which_char++;      
    }
  }
}

void printLines() {
  M5.Lcd.clear();

  for(uint8_t line = FIRST_DISP_LINE; line <= LAST_DISP_LINE; line++) {
    printLine(line);
  }
}

void loop() {

  if((byte_read = getByte()) > 0) {
    handleKey(byte_read);
  }
  if(getKeyboardInput(key_read)) {
    sendByte(translateKeyForSend(key_read));
  }
}
