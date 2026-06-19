#include <FastLED.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "SoundManager.h"
#include "TiraMatch.h"

#define LED_PIN     5
#define NUM_LEDS    288
#define BRIGHTNESS  50
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

GameBase* currentGame;

void setup() {
  Serial.begin(115200);

  SoundManager::begin();
  display.begin();

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  currentGame = new TiraMatch(leds, NUM_LEDS, &display);
  currentGame->begin();
}

void loop() {
  SoundManager::update();
  currentGame->update();
}
