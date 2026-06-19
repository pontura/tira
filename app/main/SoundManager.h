#pragma once
#include <Arduino.h>

#define BUZZER_PIN 18  // GPIO para el buzzer pasivo

class SoundManager {
public:
  static void begin();
  static void play(const char* rtttl);  // dispara una melodía RTTTL
  static void update();                  // llamar en cada loop()
  static void stop();
  static bool isPlaying();
};
