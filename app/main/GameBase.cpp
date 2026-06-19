#include "GameBase.h"

GameBase::GameBase(CRGB* leds, int numLeds, U8G2* display)
  : leds(leds), numLeds(numLeds), display(display) {}

void GameBase::setLed(int index, CRGB color) {
  if (index >= 0 && index < numLeds)
    leds[index] = color;
}

void GameBase::showLeds() {
  FastLED.show();
}

void GameBase::clearLeds() {
  FastLED.clear();
  FastLED.show();
}

void GameBase::displayClear() {
  display->clearBuffer();
}

void GameBase::displayText(int x, int y, const char* text, const uint8_t* font) {
  display->setFont(font);
  display->drawStr(x, y, text);
}

void GameBase::displaySend() {
  display->sendBuffer();
}
