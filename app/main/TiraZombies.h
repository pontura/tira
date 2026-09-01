#pragma once
#include "GameBase.h"

#define TZ_UPDATE_MS   16
#define TZ_TILT_DEAD   800     // raw units below which tilt is ignored
#define TZ_MAX_SPEED   0.15f   // LEDs per ms
#define TZ_ACCEL       0.002f  // speed added per ms per unit of normalized tilt
#define TZ_FRICTION    0.90f   // velocity multiplied each frame

class TiraZombies : public GameBase {
public:
  TiraZombies(CRGB* leds, int numLeds, U8G2* display);
  void begin()  override;
  void update() override;
  void onAnalog(int player, int16_t tiltX, int16_t tiltY, int16_t tiltZ) override;

private:
  float         pos[2];
  float         vel[2];
  float         tilt[2];       // normalized -1..1, updated by onAnalog
  unsigned long lastUpdate;

  void renderFrame();
};
