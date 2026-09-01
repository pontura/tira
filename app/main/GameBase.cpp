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

// Barra superior con fondo lleno (mismo estilo que el ítem "seleccionado" del
// menú principal): "< " + nombre del juego, texto invertido sobre el fondo.
void GameBase::displayTitleBar(const char* name) {
  char buf[24];
  sprintf(buf, "< %s", name);
  display->setFont(u8g2_font_7x14B_tr);
  display->setDrawColor(1);
  display->drawBox(0, 0, 128, 16);
  display->setDrawColor(0);
  display->drawStr(4, 13, buf);
  display->setDrawColor(1);
}

void GameBase::displaySend() {
  display->sendBuffer();
}
