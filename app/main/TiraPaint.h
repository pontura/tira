#pragma once
#include "GameBase.h"

// ── Movimiento (copiado tal cual de TiraZombies) ────────────────────────
#define TP_UPDATE_MS   16
#define TP_TILT_DEAD   800     // raw units below which tilt is ignored
#define TP_MAX_SPEED   0.15f   // LEDs per ms
#define TP_ACCEL       0.002f  // speed added per ms per unit of normalized tilt
#define TP_FRICTION    0.90f   // velocity multiplied each frame

// ── Color por rotación en Z (roll por acelerómetro, acotado) ────────────
#define TP_ROLL_MIN_DEG  -45.0f
#define TP_ROLL_MAX_DEG   45.0f

#define TP_MAX_LEDS 144

class TiraPaint : public GameBase {
public:
  TiraPaint(CRGB* leds, int numLeds, U8G2* display);
  void begin()  override;
  void update() override;
  void onInput(int player, int button)     override;
  void onButtonUp(int player, int button)  override;
  void onAnalog(int player, int16_t tiltX, int16_t tiltY, int16_t tiltZ) override;

private:
  CRGB          canvas[TP_MAX_LEDS];  // lienzo pintado, persiste entre frames
  float         pos[2];
  float         vel[2];
  float         tilt[2];              // normalized -1..1, updated by onAnalog
  uint8_t       hue[2];               // color actual del player (0-255), por rotación Z
  bool          painting[2];          // true mientras el botón está apretado
  int           prevLedPos[2];        // último LED entero ocupado (para pintar el tramo recorrido)
  unsigned long lastUpdate;

  void renderFrame();
};
