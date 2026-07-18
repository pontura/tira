#pragma once
#include "TiraMatch.h"

#define TG_FIRE_THRESHOLD   5000  // delta raw tiltY entre frames para reconocer gesto de disparo
#define TG_FIRE_COOLDOWN_MS  400  // ms mínimo entre disparos consecutivos

class TiraGestures : public TiraMatch {
public:
  TiraGestures(CRGB* leds, int numLeds, U8G2* display);
  void begin()   override;
  void onInput(int player, int button) override;
  void onAnalog(int player, int16_t tiltX, int16_t tiltY) override;

private:
  int16_t       prevTilt[2];
  bool          firstAnalog[2];
  unsigned long lastFire[2];
};
