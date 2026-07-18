#include "TiraGestures.h"

TiraGestures::TiraGestures(CRGB* leds, int numLeds, U8G2* display)
  : TiraMatch(leds, numLeds, display) {}

void TiraGestures::begin() {
  TiraMatch::begin();
  prevTilt[0]    = prevTilt[1]    = 0;
  firstAnalog[0] = firstAnalog[1] = true;
  lastFire[0]    = lastFire[1]    = 0;
}

void TiraGestures::onInput(int player, int button) {
  if (introActive || gameOver) return;
  if (button == 0) colorChange(player, +1);
  else             colorChange(player, -1);
}

void TiraGestures::onAnalog(int player, int16_t /*tiltX*/, int16_t tiltY) {
  if (player < 1 || player > 2) return;
  if (introActive || gameOver) return;
  int p = player - 1;

  // Disparo: gesto brusco de muñeca en tiltY (delta alto entre frames de 32 ms)
  if (firstAnalog[p]) {
    prevTilt[p]    = tiltY;
    firstAnalog[p] = false;
    return;
  }
  int delta   = (int)tiltY - (int)prevTilt[p];
  prevTilt[p] = tiltY;
  if (delta < -TG_FIRE_THRESHOLD &&
      millis() - lastFire[p] > TG_FIRE_COOLDOWN_MS) {
    lastFire[p] = millis();
    fire(player);  // auto-cicla el color al disparar (comportamiento TiraMatch)
  }
}
