#include "ANSITerminal.h"
#include <Arduino.h>
#include <WiFiClient.h>
#include "KeyboardHandler.h"
#include "ScreenRenderer.h"

ANSITerminal::ANSITerminal(KeyboardHandler* keyboard, ScreenRenderer* screen, WiFiClient* client,
    DebugUtil* logger) {
  this->keyboard = keyboard;
  this->screen = screen;
  this->netClient = client;
  this->logger = logger;
}

// Announce that we are 
void ANSITerminal::sendTermCap() {
  

  

}

/***************************************************
* set aside memory for rows and columns
* e.g., if 20 is passed for rows
*   21 rows created, 1 blank row not displayed, used 
8   USED FOR SCROLLING.
***************************************************/ 
bool ANSITerminal::init(uint16_t rows, uint16_t cols) {
  logger->debug("Initializing ANSI Terminal...");
  num_rows = rows + 1;
  num_cols = cols;
  first_display_row =  0; 
  current_row = first_display_row;
  last_display_row = rows - 1;
  initDisplayBuffer();
  sendTermCap();
  logger->debug("Terminal has " + String(num_cols) + " columns and " + String(num_rows)
    + " total rows, first display row is " + String(first_display_row)
    + ", current_row = " + String(current_row));

  logger->debug("Completed initialization of Terminal");
  return true;
}


uint8_t ANSITerminal::getDisplayRow(uint16_t buffer_row) {
  return buffer_row - first_display_row;
}

void ANSITerminal::initDisplayBuffer() {
  logger->debug("\tSetting aside memory for Term Buffer...");
  disp_buffer = new uint16_t[num_rows * num_cols];

  logger->debug("\tInitialing buffer characters tp spaces");
  for(uint16_t x; x < (num_rows * num_cols); x++) {
    disp_buffer[x] = combineCharAndColor(' ', screen->getIntFromColor(ScreenRenderer::ESC_DEFAULT));
  }
}

void ANSITerminal::disposeDisplayBuffer() {
  delete[] disp_buffer;
}

void ANSITerminal::initEscSequence() {
  for(int x = 0; 0 < MAX_ESC_SEQ_LEN;x++) {
    esc_sequence[x] = '\0';
  }
  esc_sequence[MAX_ESC_SEQ_LEN] = '\0';
}

bool ANSITerminal::isESCFinalByte(uint8_t key) {
  return (key >= uint8_t('A') and key <= uint8_t('Z'))
    || (key >= uint8_t('a') and key <= uint8_t('z'));
}

void ANSITerminal::handleEscapeSequence() {
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

  logger->debug("\tEscape sequence: ", false);
  logger->debug("ESC", false);
  logger->debug(esc_sequence);
  processEscapeSequence(esc_sequence);
}



bool ANSITerminal::isCursorCommand(String escSequence) {
  char last_char = escSequence.charAt(escSequence.length() - 1);
  return ESC_CURSOR_TERMINAL_CHARS.indexOf(last_char) >= 0;
}

ANSITerminal::cursorPos ANSITerminal::parseEscapeCursorValues(String escSequence) {
  cursorPos ret;

  logger->info("\t**** Parsing cursor command: " + escSequence);

  if(escSequence.length() <= 1 ) {
    ret.line = first_display_row;
    ret.column = 0;
    logger->info("\t**** Returning defaults.");
    return ret;
  } else {
    String strNums = escSequence;
    // We don't want '[' or termninating chacter like 'A'
    if(escSequence.charAt(0) == '[') {
      strNums = escSequence.substring(1, escSequence.length() - 1);
    } else {
      strNums = escSequence.substring(0, escSequence.length() - 1);
    }  

    logger->info("\t**** Parsing cursor numbers: " + strNums);

    uint8_t pos = strNums.indexOf(';');
    if( pos < 0) {
      // Assume only 1 value provided.Default the the column to current
      ret.line = parseEscNumber(escSequence) - 1 + first_display_row;
      // Row and column are 1-based with ansi escape sequence
      ret.column = current_col;
      logger->info("\t**** Only line provided, line parsed = ", false);
      logger->info(String(ret.line), false);
      logger->info(", current col = ", false);
      logger->info(String(ret.column));
      return ret; 
    } else {
      if(pos == 0) {
        // Wew skipped the line number, only column provided
        ret.line = current_row;
        ret.column = parseEscNumber(escSequence.substring(1, escSequence.length() - 1)) - 1;
        logger->info("\t**** Only column provided, column parsed = ", false);
        logger->info(String(ret.column), false);
        logger->info(", current line = ", false);
        logger->info(String(ret.line));
        return ret;
      } else {
        String strRow = strNums.substring(0, pos);
        String strCol = strNums.substring(pos + 1);

        logger->info("\t**** Parsed row and column strings: row =", false);
        logger->info(strRow, false);
        logger->info(", column = ", false);
        logger->info(strCol);

        ret.line = safeParseNumber(strRow) - 1 + first_display_row;
        ret.column = safeParseNumber(strCol) - 1;

        logger->info("\t**** Final actual cursor numbers, row = ", false);
        logger->info(String(ret.line), false);
        logger->info(", Column = ", false);
        logger->info(String(ret.column));
        return ret;
      }
    }
  }
}

uint8_t ANSITerminal::safeParseNumber(String strVal) {
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

uint8_t ANSITerminal::parseEscNumber(String escSequence) {
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

bool ANSITerminal::isColorCommand(String escSequence) {
  char last_char = escSequence.charAt(escSequence.length() - 1);
  return ESC_COLOR_TERMINAL_CHARS.indexOf(last_char) >= 0;
}


bool ANSITerminal::isEraseCommand(String escSequence) {
  char last_char = escSequence.charAt(escSequence.length() - 1);
  return ESC_ERASE_TERMINAL_CHARS.indexOf(last_char) >= 0;
}


void ANSITerminal::sendByte(uint8_t byte) {
  netClient->write(byte);
}

uint8_t ANSITerminal::getByte() {
  if(netClient->available()) {
    return uint8_t(netClient->read());
  }

  return 0;
}

// Will try to get next key for sequence with retries
uint8_t ANSITerminal::getNextByte() {
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

bool ANSITerminal::getKeyboardInput(uint8_t& key) {
  uint8_t key_val = keyboard->getKey();  // receive a byte as character

  if(key_val > 0) {
    key = key_val;
    return true;
  }       

  return false;
}


void ANSITerminal::saveCursor() {
  saved_cursor_pos.line = current_row - first_display_row;
  saved_cursor_pos.column = current_col;
}

void ANSITerminal::restoreCursor() {
  screen->eraseCursor(current_row, current_col);

  current_row = saved_cursor_pos.line + first_display_row;
  current_col = saved_cursor_pos.column;

  screen->drawCursor(current_row, current_col);
}

void ANSITerminal::printCurrentChar() {
  logger->info("\tOutputting to screen...");
  uint16_t int_val = disp_buffer[current_row * num_cols + current_col];
  char the_char = getCharFromInt(int_val);
  uint8_t color = getColorFromInt(int_val);

  if(current_text_color != color) {
    logger->info("\tChanging text color...");
    screen->setTextColor(screen->getTermColorFromInt(color));
    current_text_color = color;
  }
  
  uint16_t display_row = getDisplayRow(current_row);
  logger->info("\tCall to screen->printCharacter...");
  screen->printCharacter(the_char, display_row, current_col);
}

void ANSITerminal::copyRow(uint8_t srcRow, uint8_t destRow) {
  for(uint8_t col = 0; col < num_cols;col++) {
    disp_buffer[(destRow * num_cols) + col] = disp_buffer[(srcRow * num_cols) + col]; 
  }
}

void ANSITerminal::initRow(uint8_t row) {
  if(row >= num_rows) {
    logger->warn("**** Attempt to init row beyond end of array, ROW = : ", false);
    logger->warn(String(row));
  } else {
    for(uint8_t col = 0; col < num_cols; col++) {
      uint16_t pos = (row * num_cols) + col;

      disp_buffer[pos] = combineCharAndColor(' ', screen->getIntFromColor(ScreenRenderer::ESC_DEFAULT));
    } 
  }
}

/*
  Shift contents of all rows up 1. 
  Row 0 will be replaced by 1, 1 by 2, etc
  Last rows will be initialized with spaces.
*/
void ANSITerminal::shiftRowsUp() {
  logger->debug("Shifting rows up");
  for(uint8_t row = 1; row < num_rows; row++) {
    copyRow(row, row-1);
  }
  initRow(num_rows - 1);
}

// Copy rows 1-num_rows - 1 to
// to rows 0 - num_rows - 2  
void ANSITerminal::handleScroll() {
  logger->debug("\tTerminal needs to scroll.");
  screen->eraseCursor(getDisplayRow(current_row), current_col);
  shiftRowsUp();
  screen->scroll();
  //printLines();
}

void ANSITerminal::handlePrintingChar(uint8_t key) {
  logger->info("\tHandling printer character: ", false);
  logger->info(String(char(key)));

  logger->info("\tErasing cursor,,,");
  screen->eraseCursor(getDisplayRow(current_row), current_col);
  if(current_col > num_cols - 1) {
    current_row++;
    current_col = 0;

    if(current_row > last_display_row) {
      handleScroll();
      current_row = last_display_row;
    } 
  }

  logger->info("\tSaving character to array...");
  disp_buffer[current_row * num_cols + current_col] = combineCharAndColor(char(key), current_text_color);
  printCurrentChar();
  current_col++;
  logger->info("\tDrawing cursor...");
  screen->drawCursor(getDisplayRow(current_row), current_col);
}

void ANSITerminal::handleReturnKey() {
  screen->eraseCursor(getDisplayRow(current_row), current_col);
  current_row++;

  current_col = 0;

  if(current_row > last_display_row) {
    current_row = last_display_row;
    handleScroll();
  }  else {
    //printCurrentChar();
  }
  screen->drawCursor(getDisplayRow(current_row), current_col);
}

void ANSITerminal::handleTabKey() {
  for(int x = 0; x < TAB_SPACES; x++) {
    handleKey(VK_SPACE);
  }
}

int16_t ANSITerminal::combineCharAndColor( char c, uint8_t color) {
  return uint16_t(256) * color + c;
}

char ANSITerminal::getCharFromInt(uint16_t val) {
  return char(val & CHAR_MASK);
}

uint8_t ANSITerminal::getColorFromInt(uint16_t val) {
  return (val & COLOR_MASK) / 256;
}

void ANSITerminal::handleVkErase() {
  uint16_t erase_row = getDisplayRow(current_row);
  screen->eraseCursor(erase_row, current_col);
  disp_buffer[current_row * num_cols + current_col] = ' ';
  screen->printCharacter(' ', erase_row, current_col);
  screen->drawCursor(erase_row, current_col);  
}

void ANSITerminal::handleBackspace() {
  if(current_col > 0) {
    uint16_t erase_row = getDisplayRow(current_row);
    screen->eraseCursor(erase_row, current_col);
    current_col--;
    //do not erase
    //disp_buffer[current_row * num_cols + current_col] = ' ';
    screen->printCharacter(  disp_buffer[current_row * num_cols + current_col], erase_row, current_col);
    screen->drawCursor(erase_row, current_col);  
  }
}




// Line is 
void ANSITerminal::moveAbsolutePosition(uint8_t line, uint8_t column) {
  logger->debug("\tMove Absolute: " + String(line) + "," + String(column));
  screen->eraseCursor(getDisplayRow(current_row), current_col);
  current_row = line;
  
  if(column > (num_cols - 1)) {
    current_col = num_cols - 1;
  } else {
    if(column < 0) {
      current_col = 0;
    } else {
      current_col = column;
    }
  }

    screen->drawCursor(getDisplayRow(current_row), current_col); 
}


void ANSITerminal::moveRelativePosition(MoveCursorDir direction, uint8_t amount) {
  logger->info("\t handling Move relative: ", false);
  logger->info(getCursorDirDesc(direction), false);
  logger->info(" - ", false);
  logger->info(String(amount));


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
  logger->debug("\tCompleted move relative.");
}

void ANSITerminal::clearScreen() {
  initDisplayBuffer();
  
  current_row = first_display_row;
  current_col = 0;
  
  screen->clearScreen();
  screen->drawCursor(getDisplayRow(current_row), current_col); 
}

void ANSITerminal::handleColorCommand(String sequence) {
  logger->debug("***** Handling Color Command: " + sequence);
  int char_pos = sequence.indexOf(';');

  if(char_pos > 0) {
    // Should be something like 01;34m
    String my_color = sequence.substring(char_pos + 1, sequence.length() - 1);
    uint8_t esc_color = my_color.toInt();
    Serial.print(", COLOR = ");
    Serial.println(esc_color);
    
   
    current_text_color =  screen->getTermColorFromInt(esc_color);
  } else {
    Serial.println(", RESET COLOR");
    screen->setTextColor(ScreenRenderer::ESC_DEFAULT);
    current_text_color = ScreenRenderer::ESC_DEFAULT;
  }
}

void ANSITerminal::hideShowCursor(String escSequence) {
  uint16_t cursor_row = getDisplayRow(current_row);
  if(escSequence.charAt(escSequence.length() - 1) == 'h') {
    screen->drawCursor(cursor_row, current_col);
  } else {
    screen->eraseCursor(cursor_row, current_col);
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

void ANSITerminal::handleCursorCommand(String escSequence) {
  logger->debug("\tHandling Cursor command: " + escSequence);
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
      logger->warn("Unahandled Cursor Command: " + escSequence);
      break;
  }

  logger->debug("\t completed cursor command.");
}

void ANSITerminal::swap(uint8_t* val1, uint8_t* val2) {
  uint8_t tmp = *val1;
  *val1 = *val2;
  *val2 = tmp;
}


void ANSITerminal::doErase(
    uint8_t start_line,
    uint8_t end_line,
    uint8_t start_col,
    uint8_t end_col
 ) {
  logger->debug("Erasing from line ", false);
  logger->debug(String(start_line), false);
  logger->debug(" to line ", false);
  logger->debug(String(end_line), false);
  logger->debug(", col ", false);
  logger->debug(String(start_col), false);
  logger->debug(" to ", false);
  logger->debug(String(end_col));
  
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

  if(start_line > num_rows -1) {
    start_line = num_rows - 1;
  }

  if(end_col > num_cols - 1) {
    end_col = num_cols - 1;
  }
  uint8_t myLine = start_line;
  uint8_t myCol = start_col;

  for(uint8_t row = start_line; row <= end_line; row++) {
    for(uint8_t col = start_col; col <= end_col;col++) {
      disp_buffer[row * num_cols + col] = combineCharAndColor(' ', screen->getIntFromColor(ScreenRenderer::ESC_DEFAULT));
    }
  }
}

String ANSITerminal::getCursorDirDesc(ANSITerminal::MoveCursorDir ctype) {
  switch(ctype) {
    case DOWN:
      return "Down";
    case UP:
      return "Up";
    case LEFT:
      return "Left";
    case RIGHT:
      return "Right";
    default:
    return "N/A";
  }
}

String ANSITerminal::getEraseTypeDesc(ANSITerminal::eraseType etype) {
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
    default:
    return "N/A";
  }
}

void ANSITerminal::processEraseCommand(eraseType erase_type) {
  logger->debug("Erasing: " + getEraseTypeDesc(erase_type));
  screen->eraseCursor(getDisplayRow(current_row), current_col);

  switch(erase_type) {
    case END_OF_LINE:
      doErase(current_row, 
        current_row, 
        current_col, 
        num_cols);
      printLine(current_row);
      break;
    case BEGINNING_OF_LINE:
      doErase(current_row, 
        current_row, 
        0, 
        current_col);
      printLine(current_row);
      break;
    case END_OF_SCREEN:
      doErase(current_row, 
        current_row, 
        current_col, 
        num_cols - 1);
      doErase(current_row + 1, 
        last_display_row, 
        0, 
        num_cols - 1);
      printLines();
      break;
    case BEGINNING_OF_SCREEN:
      doErase(0 ,
        current_row - 1, 
        0, 
        num_cols - 1);
      doErase(current_row , 
        current_row, 
        0, 
        current_col);
      printLines();
      break;
    case ENTIRE_SCREEN:
      doErase(first_display_row,
        last_display_row, 
        0, 
        num_cols - 1);
        printLines(); 
  }

  logger->debug("Completed erasing.");
  screen->drawCursor(getDisplayRow(current_row), current_col);
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

void ANSITerminal::handleEraseCommand(String escSequence) {
  logger->debug("\tHandling erase command: " + escSequence);
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
      logger->warn("Unhandled erase esc sequence: " + escSequence);
      break;
  }

  logger->debug("\tDone erasing.");
}

void ANSITerminal::processEscapeSequence(String escSequence) {
  logger->debug("\tProcessing escSequence: " + escSequence);
  if(isColorCommand(escSequence)) {
    handleColorCommand(escSequence);
  } else {
    if(isCursorCommand(escSequence)) {
      handleCursorCommand(escSequence);
    } else {
      if(isEraseCommand(escSequence)) {
        handleEraseCommand(escSequence);
      } else {
        logger->warn("Unhandled Escape Sequnce: " + escSequence);
      }
    }
  }


  logger->debug("Esc Sequence completed: " + escSequence);
}

void ANSITerminal::handleControlChar(uint8_t key) {
  logger->info("\tHandling control character: ");
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
    default:
      logger->warn("Control Character:  ", false);
      logger->warn(String(key));
      logger->warn(" - Unhandled");
  }
}

bool ANSITerminal::isControlChar(uint8_t key) {
  return key < 32 || VK_DELETE == key || VK_TAB == key;
}


void ANSITerminal::handleKey(uint8_t key) {
  logger->info("\tHandling key...");


  if(isControlChar(key)) {
    handleControlChar(key);
  } else {
    handlePrintingChar(key);
  }

  last_key = key;
}

void ANSITerminal::printLine(uint8_t  row) {
  int which_char = 0;
  uint16_t the_val;
  char c;
  uint8_t char_color;
  uint8_t last_color = ESC_WHITE;
  bool is_null_char = false;

  
  logger->info("Printing display row " +  String(getDisplayRow(row)));

  while(which_char <  num_cols) {
    the_val = disp_buffer[row * num_cols + which_char];
    c = getCharFromInt(the_val);
    char_color = getColorFromInt(the_val);
    
    if(last_color != char_color) {
      screen->setTextColor(screen->getTermColorFromInt(char_color));
      last_color = char_color;
    }

    if('\0' == c) {
      is_null_char = true; // Don't save it
    } else {
      screen->printCharacter(c, getDisplayRow(row), which_char);
      which_char++;      
    }
  }
}

void ANSITerminal::printLines() {
  screen->clearScreen();

  for(uint8_t line = first_display_row; line <= last_display_row; line++) {
    printLine(line);
  }
}

void ANSITerminal::loop() {

  if((byte_read = getByte()) > 0) {
    logger->info("Byte read from network", false);
    logger->info(String(byte_read));
    handleKey(byte_read);
  }

  if(getKeyboardInput(key_read)) {
    logger->info(String(key_read));
    sendByte(keyboard->translateKey(key_read));
  }
}
