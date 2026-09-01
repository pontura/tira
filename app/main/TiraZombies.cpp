#include "TiraZombies.h"
#include <math.h>

TiraZombies::TiraZombies(CRGB* leds, int numLeds, U8G2* display)
  : GameBase(leds, numLeds, display) {}

void TiraZombies::begin() {
  pos[0] = numLeds / 2.0f - 4.0f;
  pos[1] = numLeds / 2.0f + 4.0f;
  vel[0] = vel[1] = 0.0f;
  tilt[0] = tilt[1] = 0.0f;
  lastUpdate = millis();
  renderFrame();
}

void TiraZombies::onAnalog(int player, int16_t /*tiltX*/, int16_t tiltY, int16_t /*tiltZ*/) {
  if (player < 1 || player > 2) return;
  int p = player - 1;
  if (abs(tiltY) < TZ_TILT_DEAD) {
    tilt[p] = 0.0f;
  } else {
    tilt[p] = constrain(tiltY / 16384.0f, -1.0f, 1.0f);
  }
}

void TiraZombies::update() {
  unsigned long now = millis();
  unsigned long dt  = now - lastUpdate;
  if (dt < TZ_UPDATE_MS) return;
  lastUpdate = now;

  for (int p = 0; p < 2; p++) {
    if (tilt[p] == 0.0f) {
      vel[p] *= 0.80f;  // frena rápido cuando el control está quieto
      if (fabsf(vel[p]) < 0.001f) vel[p] = 0.0f;
    } else {
      vel[p] += tilt[p] * TZ_ACCEL * dt;
      vel[p] *= TZ_FRICTION;
      vel[p]  = constrain(vel[p], -TZ_MAX_SPEED, TZ_MAX_SPEED);
    }
    pos[p] += vel[p] * dt;
    float lo = 1.0f, hi = (float)(numLeds - 2);
    if (pos[p] < lo)      pos[p] = hi;
    else if (pos[p] > hi) pos[p] = lo;
  }

  renderFrame();
}

void TiraZombies::renderFrame() {
  clearLeds();
  int p0 = (int)round(pos[0]);
  int p1 = (int)round(pos[1]);
  setLed(constrain(p0 - 1, 0, numLeds - 1), CRGB::Red);
  setLed(constrain(p0,     0, numLeds - 1), CRGB::Red);
  setLed(constrain(p1,     0, numLeds - 1), CRGB::Blue);
  setLed(constrain(p1 + 1, 0, numLeds - 1), CRGB::Blue);
  showLeds();
}
