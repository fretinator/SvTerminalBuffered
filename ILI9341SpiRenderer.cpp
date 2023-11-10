#include "ILI9341SpiRenderer.h"

ILI9341SpiRenderer::ILI9341SpiRenderer(uint8_t textSize, TermColor backcolor, TermColor forecolor,
    DebugUtil* logger) {
  this->text_size = text_size;
  this->backcolor = backcolor;
  this->forecolor = forecolor;
  this->logger = logger;
}

void ILI9341SpiRenderer::printCharacter(char c, uint16_t row, uint16_t col) {
  uint16_t x_pos = getColX(col);
  uint16_t y_pos = getRowY(row);

  logger->debug("Printing character " +
  String(c) + " from row " +
  String(row) + " and col " + 
  String(col) + " at x=" +
  String(x_pos) + " and y=" + 
  String(y_pos));

  screen.fillRect(x_pos, y_pos, text_width, text_height, mapColor(backcolor));
  screen.drawChar(x_pos, y_pos, c, mapColor(forecolor), mapColor(backcolor), text_size);
}

void ILI9341SpiRenderer::printString(String str, uint16_t row, uint16_t col) {
  uint16_t x_pos = getRowY(row);
  uint16_t y_pos = getColX(col);

  screen.setCursor(x_pos, y_pos);
  screen.fillRect(x_pos, y_pos, screen_w - x_pos, text_height, ILI9341_BLUE);
  screen.setTextColor(mapColor(forecolor));
  screen.print(str);
}

void ILI9341SpiRenderer::clearScreen() {
  fillscreen();
}

void ILI9341SpiRenderer::setBackColor(TermColor color) {
  backcolor = color;
}

void ILI9341SpiRenderer::setTextColor(TermColor color) {
  forecolor = color;
}


void ILI9341SpiRenderer::fillscreen() {
  screen.fillScreen(mapColor(backcolor));
}

void ILI9341SpiRenderer::drawCursor(uint16_t row, uint16_t col) {
  uint16_t x_pos = getColX(col);
  uint16_t y_pos = getRowY(row); ;
  screen.drawRect(x_pos, y_pos, text_width, text_height, ILI9341_BLUE);
}

void ILI9341SpiRenderer::eraseCursor(uint16_t row, uint16_t col) {
  uint16_t x_pos = getColX(col);
  uint16_t y_pos = getRowY(row);
  screen.drawRect(x_pos, y_pos, text_width, text_height, mapColor(backcolor));
}

uint16_t ILI9341SpiRenderer::getHeight() {
  return screen_h;
}

uint16_t ILI9341SpiRenderer::getWidth() {
  return screen_w;
}

uint8_t ILI9341SpiRenderer::getTextHeight() {
  return text_height;
}

uint8_t ILI9341SpiRenderer::getTextWidth() {
  return text_width;
} 


uint16_t ILI9341SpiRenderer::getRowY(uint16_t row) {
  uint16_t ret = row * text_height + yOffset;

  if(ret >= screen_h) {
    ret -= screen_h;
  }

  return ret;
}

uint16_t ILI9341SpiRenderer::getColX(uint16_t col) {
  uint16_t ret = (col * text_width) + SCREEN_INDENT;

  return ret;
}

uint16_t ILI9341SpiRenderer::mapColor(TermColor color) {
 switch(color) {
    case ESC_RESET:
      return ILI9341_BLACK;
    case ESC_BLACK:
      return ILI9341_BLACK;
    case ESC_RED:
      return ILI9341_RED;
    case ESC_GREEN:
      return ILI9341_GREEN;
    case ESC_YELLOW:
      return ILI9341_YELLOW;
    case ESC_BLUE:
      return ILI9341_BLUE;
    case ESC_MAGENTA:
      return ILI9341_MAGENTA;
    case ESC_CYAN:
      return ILI9341_CYAN;
    case ESC_WHITE:
      return ILI9341_WHITE;
    default:
      return ILI9341_WHITE;
  }
}

uint16_t ILI9341SpiRenderer::getRows() {
  return rows;
}

uint16_t ILI9341SpiRenderer::getCols() {
  return cols;
}

bool ILI9341SpiRenderer::init() {
  logger->debug("Initializing screen...");
  
  screen.begin();
  screen.setRotation(0);
  
  Serial.println("\tSetting colors for screen");
  screen.fillScreen(mapColor(backcolor));
  screen.setTextColor(mapColor(forecolor));
  
  Serial.println("\tGetting dimensions of screen and text");
  screen_w = screen.width() - SCREEN_INDENT;
  screen_h = screen.height();

  screen.setCursor(0,0);
String f_test = "Wg";
  int16_t f_x,f_y;
  uint16_t f_width, f_height;

  screen.getTextBounds(f_test, 0,0, &f_x, &f_y, &f_width, &f_height);

  text_height = f_height + LINE_PAD;
  text_width = (f_width / 2) + COL_PAD;

  rows = screen_h / text_height;
  cols = screen_w / text_width;

  logger->debug("Screen: Height = " + String(screen_h) + ", Width = " + String(screen_w) 
    + ", text_height = " + String(text_height) + ", text_width = " + String(text_width)
    + ", rows=" + String(rows) + ", cols=" + String(cols));

  screen.print("TFT Screen Initialized");

  logger->debug("Screen initialized");
  return true;
}

bool ILI9341SpiRenderer::scroll() {
  scroll_line();
  return true;
}

int ILI9341SpiRenderer::scroll_line() {
  // Use the record of line lengths to optimise the rectangle size we need to
    // erase the top line
    // M5.Lcd.fillRect(0,yOffset,blank[(yOffset-TOP_FIXED_AREA)/TEXT_HEIGHT],TEXT_HEIGHT,
    // TFT_BLACK);
    screen.fillRect(0, yOffset, 320, text_height, ILI9341_BLACK);

    // Change the top of the scroll area
    yOffset += text_height;
    // The value must wrap around as the screen memory is a circular buffer
    // if (yOffset >= YMAX - BOT_FIXED_AREA) yOffset = TOP_FIXED_AREA + (yOffset -
    // YMAX + BOT_FIXED_AREA);
    if (yOffset >= screen_h) {
      yOffset = 0;
    }
    // Now we can scroll the display
    screen.scrollTo(yOffset);
    //scrollAddress(yOffset);

    Serial.printf("*** Completed scroll_line, yOffset = %d \n", yOffset);

    return yOffset;
}

// ##############################################################################################
// Setup a portion of the screen for vertical scrolling
// ##############################################################################################
// We are using a hardware feature of the display, so we can only scroll in
// portrait orientation
void ILI9341SpiRenderer::setupScrollArea(uint16_t tfa, uint16_t bfa) {
    screen.setScrollMargins(0, 0);
    /*
    screen.writecommand(ILI9341_VSCRDEF);  // Vertical scroll definition
    screen.writedata(tfa >> 8);            // Top Fixed Area line count
    screen.writedata(tfa);
    screen.writedata((screen_h - tfa - bfa) >>
                     8);  // Vertical Scrolling Area line count
    screen.writedata(screen_h - tfa - bfa);
    screen.writedata(bfa >> 8);  // Bottom Fixed Area line count
    screen.writedata(bfa);
    */
}

// ##############################################################################################
// Setup the vertical scrolling start address pointer
// ##############################################################################################
void ILI9341SpiRenderer::scrollAddress(uint16_t vsp) {
    screen.setAddrWindow(0, 0, screen.width(), screen.height());
    /*
    screen.writecommand(ILI9341_VSCRSADD);  // Vertical scrolling pointer
    screen.writedata(vsp >> 8);
    screen.writedata(vsp);
    */
}
