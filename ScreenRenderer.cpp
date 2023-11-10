#include <Arduino.h>
#include "ScreenRenderer.h"


void ScreenRenderer::printCharacter(char c, uint16_t row, uint16_t col) {

}

void ScreenRenderer::printString(String str, uint16_t row, uint16_t col) {

}

void ScreenRenderer::clearScreen() {

}

void ScreenRenderer::setBackColor(TermColor color) {

}

void ScreenRenderer::setTextColor(TermColor color) {

}

void ScreenRenderer::fillScreen(uint8_t color) {

}

void ScreenRenderer::drawCursor(uint16_t row, uint16_t col) {

}

void ScreenRenderer::eraseCursor(uint16_t row, uint16_t col) {

}


uint16_t ScreenRenderer::getHeight() {
	return 0;
}

uint16_t ScreenRenderer::getWidth() {
	return 0;
}

uint8_t ScreenRenderer::getTextHeight() {
	return 0;
}

uint8_t ScreenRenderer::getTextWidth() {
	return 0;
}

uint16_t ScreenRenderer::getRows() {

}

uint16_t ScreenRenderer::getCols() {

}

bool ScreenRenderer::init() {
	return false;
}

bool ScreenRenderer::scroll() {
  return false; // means not supported
}



