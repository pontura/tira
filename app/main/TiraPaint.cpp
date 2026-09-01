#include "TiraPaint.h"
#include <math.h>

TiraPaint::TiraPaint(CRGB* leds, int numLeds, U8G2* display)
  : GameBase(leds, numLeds, display) {}

void TiraPaint::begin() {
  for (int i = 0; i < TP_MAX_LEDS; i++) canvas[i] = CRGB::Black;

  pos[0] = numLeds / 2.0f - 4.0f;
  pos[1] = numLeds / 2.0f + 4.0f;
  vel[0] = vel[1] = 0.0f;
  tilt[0] = tilt[1] = 0.0f;
  hue[0] = hue[1] = 128;
  painting[0] = painting[1] = false;
  prevLedPos[0] = (int)roundf(pos[0]);
  prevLedPos[1] = (int)roundf(pos[1]);
  lastUpdate = millis();
  renderFrame();
}

// Movimiento: idéntico a TiraZombies (tiltY controla la velocidad)
void TiraPaint::onAnalog(int player, int16_t tiltX, int16_t tiltY, int16_t tiltZ) {
  if (player < 1 || player > 2) return;
  int p = player - 1;

  if (abs(tiltY) < TP_TILT_DEAD) {
    tilt[p] = 0.0f;
  } else {
    tilt[p] = constrain(tiltY / 16384.0f, -1.0f, 1.0f);
  }

  // Color: roll por acelerómetro (X/Z), independiente del eje que mueve al player (Y)
  float rollDeg = atan2f((float)tiltX, (float)tiltZ) * 180.0f / PI;
  rollDeg = constrain(rollDeg, TP_ROLL_MIN_DEG, TP_ROLL_MAX_DEG);
  float t = (rollDeg - TP_ROLL_MIN_DEG) / (TP_ROLL_MAX_DEG - TP_ROLL_MIN_DEG);  // 0..1
  hue[p] = (uint8_t)(t * 255.0f);
}

void TiraPaint::onInput(int player, int /*button*/) {
  if (player < 1 || player > 2) return;
  painting[player - 1] = true;  // cualquiera de los 2 botones pinta
}

void TiraPaint::onButtonUp(int player, int /*button*/) {
  if (player < 1 || player > 2) return;
  painting[player - 1] = false;
}

void TiraPaint::update() {
  unsigned long now = millis();
  unsigned long dt  = now - lastUpdate;
  if (dt < TP_UPDATE_MS) return;
  lastUpdate = now;

  for (int p = 0; p < 2; p++) {
    if (tilt[p] == 0.0f) {
      vel[p] *= 0.80f;  // frena rápido cuando el control está quieto
      if (fabsf(vel[p]) < 0.001f) vel[p] = 0.0f;
    } else {
      vel[p] += tilt[p] * TP_ACCEL * dt;
      vel[p] *= TP_FRICTION;
      vel[p]  = constrain(vel[p], -TP_MAX_SPEED, TP_MAX_SPEED);
    }
    pos[p] += vel[p] * dt;

    float lo = 1.0f, hi = (float)(numLeds - 2);
    bool wrapped = false;
    if (pos[p] < lo)      { pos[p] = hi; wrapped = true; }
    else if (pos[p] > hi) { pos[p] = lo; wrapped = true; }

    int newLed = constrain((int)roundf(pos[p]), 0, numLeds - 1);

    // Pintar el tramo recorrido con el color que tenía el player al salir de cada LED
    if (painting[p] && !wrapped) {
      int a = prevLedPos[p], b = newLed;
      if (a > b) { int t = a; a = b; b = t; }
      CRGB c = CHSV(hue[p], 255, 255);
      for (int j = a; j <= b; j++) canvas[j] = c;
    }
    prevLedPos[p] = newLed;
  }

  renderFrame();
}

void TiraPaint::renderFrame() {
  for (int i = 0; i < numLeds; i++) setLed(i, canvas[i]);

  // Cursor de cada player encima del lienzo, con su color actual
  int l0 = constrain((int)roundf(pos[0]), 0, numLeds - 1);
  int l1 = constrain((int)roundf(pos[1]), 0, numLeds - 1);
  setLed(l0, CHSV(hue[0], 255, 255));
  setLed(l1, CHSV(hue[1], 255, 255));

  showLeds();
}
