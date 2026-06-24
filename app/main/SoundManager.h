#pragma once
#include <Arduino.h>

#define BUZZER_PIN 18  // GPIO para el buzzer pasivo

class SoundManager {
public:
  static void begin();
  static void play(const char* rtttl);
  static void update();
  static void stop();
  static bool isPlaying();

  // Se llama cuando se reproduce un sonido en el master (para reenviar a joysticks)
  static void setOnPlayCallback(void (*cb)(const char*));

  // Envía sonido a todos los joysticks sin reproducir en el master
  static void sendToJoysticks(const char* rtttl);
  static void setOnJoystickCallback(void (*cb)(const char*));

  // Envía sonido solo al joystick de un jugador específico
  static void sendToPlayer(uint8_t player, const char* rtttl);
  static void setOnPlayerCallback(void (*cb)(uint8_t, const char*));

private:
  static void (*onPlayCallback)(const char*);
  static void (*onJoystickCallback)(const char*);
  static void (*onPlayerCallback)(uint8_t, const char*);
};
