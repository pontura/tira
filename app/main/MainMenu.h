#pragma once
#include <U8g2lib.h>
#include <FastLED.h>
#include <Arduino.h>
#include "SoundManager.h"   // BUZZER_PIN

#define MENU_VRY_PIN      34    // GPIO — joystick Y axis analógico (0–4095)
#define MENU_VRX_PIN      35    // GPIO — joystick X axis analógico (0–4095), mover derecha = entrar
#define MENU_SW_PIN       13    // GPIO — joystick button (no usado para selección, reservado)
#define MENU_CENTER      2048   // valor ADC en reposo (HW-504 con 3.3V)
#define MENU_DEADZONE     400   // zona muerta alrededor del centro
#define MENU_SCROLL_SLOW  500   // ms entre pasos de scroll a mínima inclinación
#define MENU_SCROLL_FAST   80   // ms entre pasos de scroll a máxima inclinación

#define MENU_LED_ANIM_MS    220  // duración del desplazamiento ease-in-out entre juegos
#define MENU_LED_UPDATE_MS   16  // ms entre frames de la animación de LEDs (~60fps)

#define MENU_SELECT_BEEP_FREQ  60  // Hz — bien grave
#define MENU_SELECT_BEEP_MS    30  // ms — bien cortito

class MainMenu {
public:
  MainMenu(U8G2* display, const char** names, int count, CRGB* leds, int numLeds);
  void begin();
  int  update();   // -1 = sigue en menú; >=0 = índice del juego a lanzar

private:
  void render();
  void updateLeds();
  float segmentStart(int index) const;  // LED donde arranca el segmento de ese juego

  U8G2*         display;
  const char**  names;
  int           count;
  int           selected;
  int           scrollOffset;
  unsigned long lastScroll;
  bool          swWasPressed;
  bool          xWasActive;

  CRGB*         leds;
  int           numLeds;
  float         ledSegAnimFrom;
  float         ledSegAnimTo;
  unsigned long ledAnimStart;
  unsigned long lastLedUpdate;

  unsigned long buzzerOffAt;  // millis() en que hay que cortar el beep de cambio de juego
};
