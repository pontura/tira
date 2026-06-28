#pragma once
#include <FastLED.h>
#include <U8g2lib.h>
#include "SoundManager.h"

class GameBase {
public:
  GameBase(CRGB* leds, int numLeds, U8G2* display);
  virtual ~GameBase() {}

  virtual void begin() {}
  virtual void update() = 0;
  virtual void onInput(int player, int button) {}
  virtual void onButtonUp(int player, int button) {}

protected:
  CRGB* leds;
  int   numLeds;
  U8G2* display;

  void setLed(int index, CRGB color);
  void showLeds();
  void clearLeds();
  void displayClear();
  void displayText(int x, int y, const char* text, const uint8_t* font = u8g2_font_6x10_tr);
  void displaySend();
  void playSound(const char* rtttl)        { SoundManager::play(rtttl); }
  void sendJoystickSound(const char* rtttl)                    { SoundManager::sendToJoysticks(rtttl); }
  void sendJoystickSoundToPlayer(int player, const char* rtttl){ SoundManager::sendToPlayer(player, rtttl); }
  void stopSound()                         { SoundManager::stop(); }
};
