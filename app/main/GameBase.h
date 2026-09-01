#pragma once
#include <FastLED.h>
#include <U8g2lib.h>
#include "SoundManager.h"

// Sonido de error genérico (RTTTL), reutilizable por cualquier juego
// al penalizar a un jugador — se envía al joystick de ese jugador.
#define SND_ERROR "Error:d=8,o=4,b=100:e4,b3"

class GameBase {
public:
  GameBase(CRGB* leds, int numLeds, U8G2* display);
  virtual ~GameBase() {}

  virtual void begin() {}
  virtual void update() = 0;
  virtual void onInput(int player, int button) {}
  virtual void onButtonUp(int player, int button) {}
  virtual void onAnalog(int player, int16_t tiltX, int16_t tiltY, int16_t tiltZ) {}

protected:
  CRGB* leds;
  int   numLeds;
  U8G2* display;

  void setLed(int index, CRGB color);
  void showLeds();
  void clearLeds();
  void displayClear();
  void displayText(int x, int y, const char* text, const uint8_t* font = u8g2_font_6x10_tr);
  void displayTitleBar(const char* name);  // barra superior con fondo lleno: "< " + nombre del juego
  void displaySend();
  void playSound(const char* rtttl)        { SoundManager::play(rtttl); }
  void sendJoystickSound(const char* rtttl)                    { SoundManager::sendToJoysticks(rtttl); }
  void sendJoystickSoundToPlayer(int player, const char* rtttl){ SoundManager::sendToPlayer(player, rtttl); }
  void stopSound()                         { SoundManager::stop(); }
};
