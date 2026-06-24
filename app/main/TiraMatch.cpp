#include "TiraMatch.h"

const CRGB TiraMatch::COLORS[6] = {
  CRGB::Red, CRGB::Green, CRGB::Blue,
  CRGB::Yellow, CRGB::Cyan, CRGB::Magenta
};

TiraMatch::TiraMatch(CRGB* leds, int numLeds, U8G2* display)
  : GameBase(leds, numLeds, display) {
  spawnBuffer = new CRGB[numLeds]();
}

TiraMatch::~TiraMatch() {
  delete[] spawnBuffer;
}

void TiraMatch::begin() {
  for (int i = 0; i < MAX_SHOTS; i++) shots[i].active = false;
  fill_solid(spawnBuffer, numLeds, CRGB::Black);

  p1ColorIdx    = 0;
  p2ColorIdx    = 0;
  spawnIntervalMs = SPAWN_INTERVAL;
  spawnTurn       = 0;
  leftLedInSeg   = 0;
  rightLedInSeg  = 0;
  leftSpawnCount  = 0;
  rightSpawnCount = 0;
  activeColors   = 3;
  leftColorIdx   = random(3);
  rightColorIdx  = random(3);

  for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;
  lastParticleUpdate = millis();

  spawnCenter    = numLeds / 2;
  shiftRemaining = 0;
  shiftDir       = 0;
  gameOver       = false;
  loser          = 0;
  goPhase        = GO_NONE;
  goBlinkCount   = 0;
  goBlinkOn      = false;
  goTimer        = 0;
  goSweepStep    = 0;

  unsigned long now = millis();
  lastMove      = now;
  lastSpawn     = now;
  lastShift     = now;
  lastColorAdd  = now;
  introActive   = true;
  introStart    = now;
  lastIntroSound = now;

  clearLeds();
  updateDisplay();
  Serial.println("TiraMatch. 1/2=disparar  9/0=cambiar color");
}

void TiraMatch::update() {
  if (introActive) { updateIntro(); return; }
  if (gameOver)   { updateGameOver(); return; }

  unsigned long now = millis();

  if (activeColors < 6 && now - lastColorAdd >= 20000) {
    activeColors++;
    lastColorAdd = now;
  }

  if (Serial.available()) {
    char c = Serial.read();
    if (c == '1') onInput(1, 0);
    if (c == '2') onInput(2, 0);
    if (c == '9') onInput(1, 1);
    if (c == '0') onInput(2, 1);
  }

  if (now - lastParticleUpdate >= PARTICLE_MS) {
    lastParticleUpdate = now;
    updateParticles();
  }

  if (now - lastMove >= SHOT_SPEED) {
    lastMove = now;
    moveShots();
    checkCollisions();
  }

  if (shiftRemaining > 0) {
    if (now - lastShift >= SHIFT_STEP_MS) {
      lastShift = now;
      stepShift();
    }
  } else {
    if (now - lastSpawn >= (unsigned long)spawnIntervalMs) {
      lastSpawn = now;
      spawnNextLed();
    }
  }

  if (checkGameOver()) return;

  renderFrame();
}

void TiraMatch::onInput(int player, int button) {
  if (introActive || gameOver) return;
  if (button == 0) {
    fire(player);
  } else {
    if (player == 1) p1ColorIdx = nextColorInCycle(p1ColorIdx);
    else             p2ColorIdx = nextColorInCycle(p2ColorIdx);
    sendJoystickSoundToPlayer(player, SND_COLOR);
    updateDisplay();
  }
}

void TiraMatch::fire(int player) {
  int slot = findFreeSlot();
  if (slot < 0) return;
  sendJoystickSoundToPlayer(player, SND_SHOOT);

  Shot& s  = shots[slot];
  s.active = true;

  if (player == 1) {
    s.pos      = SHOT_LEN - 1;
    s.dir      = 1;
    s.colorIdx = p1ColorIdx;
    s.color    = COLORS[p1ColorIdx];
    p1ColorIdx = nextColorInCycle(p1ColorIdx);
  } else {
    s.pos      = numLeds - SHOT_LEN;
    s.dir      = -1;
    s.colorIdx = p2ColorIdx;
    s.color    = COLORS[p2ColorIdx];
    p2ColorIdx = nextColorInCycle(p2ColorIdx);
  }
  updateDisplay();
}

void TiraMatch::moveShots() {
  for (int i = 0; i < MAX_SHOTS; i++) {
    if (!shots[i].active) continue;
    shots[i].pos += shots[i].dir;
    if (shots[i].pos < 0 || shots[i].pos >= numLeds)
      shots[i].active = false;
  }
}

void TiraMatch::checkCollisions() {
  int center     = spawnCenter;
  int leftStart  = center - 2;
  int rightStart = center + 2;

  for (int i = 0; i < MAX_SHOTS; i++) {
    if (!shots[i].active) continue;
    int head = shots[i].pos;

    if (shots[i].dir == 1 && head <= leftStart) {
      if (spawnBuffer[head] != CRGB::Black) {
        if (spawnBuffer[head] == shots[i].color) {
          int ci        = shots[i].colorIdx;
          int extraSegs = (ci == 4) ? 1 : (ci == 5) ? 2 : 0;
          int shiftAmt  = (ci == 3) ? 20 : (ci == 4) ? 17 : (ci == 5) ? 15 : SHIFT_STEPS;
          explodeAndDestroy(head, 0, leftStart, -1);
          int searchPos = head;
          for (int s = 0; s < extraSegs; s++) {
            int nextHit = -1;
            for (int p = searchPos; p <= leftStart; p++) {
              if (spawnBuffer[p] != CRGB::Black) { nextHit = p; break; }
            }
            if (nextHit < 0) break;
            explodeAndDestroy(nextHit, 0, leftStart, -1);
            searchPos = nextHit;
          }
          startShift(1, shiftAmt);
        } else {
          addPenalty(1, shots[i].color);
        }
        shots[i].active = false;
      }
    }

    if (shots[i].dir == -1 && head >= rightStart) {
      if (spawnBuffer[head] != CRGB::Black) {
        if (spawnBuffer[head] == shots[i].color) {
          int ci        = shots[i].colorIdx;
          int extraSegs = (ci == 4) ? 1 : (ci == 5) ? 2 : 0;
          int shiftAmt  = (ci == 3) ? 20 : (ci == 4) ? 17 : (ci == 5) ? 15 : SHIFT_STEPS;
          explodeAndDestroy(head, rightStart, numLeds - 1, 1);
          int searchPos = head;
          for (int s = 0; s < extraSegs; s++) {
            int nextHit = -1;
            for (int p = searchPos; p >= rightStart; p--) {
              if (spawnBuffer[p] != CRGB::Black) { nextHit = p; break; }
            }
            if (nextHit < 0) break;
            explodeAndDestroy(nextHit, rightStart, numLeds - 1, 1);
            searchPos = nextHit;
          }
          startShift(-1, shiftAmt);
        } else {
          addPenalty(2, shots[i].color);
        }
        shots[i].active = false;
      }
    }
  }
}

// Encuentra el segmento contiguo, lanza la explosión y lo destruye
void TiraMatch::explodeAndDestroy(int hitIdx, int minBound, int maxBound, int explosionDir) {
  CRGB hitColor = spawnBuffer[hitIdx];
  int  start = hitIdx, end = hitIdx;

  while (start > minBound && spawnBuffer[start - 1] == hitColor) start--;
  while (end   < maxBound && spawnBuffer[end   + 1] == hitColor) end++;

  int count  = min(end - start + 1, (int)SEGMENT_LEN);
  int player = (explosionDir == -1) ? 1 : 2;
  spawnExplosion(start, count, explosionDir);
  sendJoystickSoundToPlayer(player, SND_EXPLOSION);

  for (int i = start; i < start + count; i++)
    spawnBuffer[i] = CRGB::Black;
}

// Crea una partícula por cada LED del segmento destruido
void TiraMatch::spawnExplosion(int segStart, int segLen, int dir) {
  for (int i = 0; i < segLen; i++) {
    int slot = findFreeParticle();
    if (slot < 0) break;

    Particle& p = particles[slot];
    p.active = true;
    p.pos    = segStart + i;
    p.vel    = random(225, 901) / 100.0f;  // 2.25 a 9.0 LEDs/frame
    p.maxVel = p.vel;
    p.dir    = dir;
  }
}

void TiraMatch::updateParticles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) continue;

    particles[i].pos += particles[i].vel * particles[i].dir;
    particles[i].vel *= PARTICLE_DECEL;

    if (particles[i].vel < 0.1f ||
        particles[i].pos < 0 ||
        particles[i].pos >= numLeds)
      particles[i].active = false;
  }
}

void TiraMatch::renderParticles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) continue;
    int idx = (int)particles[i].pos;
    if (idx < 0 || idx >= numLeds) continue;

    uint8_t brightness = (uint8_t)(particles[i].vel / particles[i].maxVel * 255.0f);
    leds[idx] = CRGB(brightness, brightness, brightness);
  }
}

int TiraMatch::findFreeParticle() {
  for (int i = 0; i < MAX_PARTICLES; i++)
    if (!particles[i].active) return i;
  return -1;
}

// Spawn alternado: un lado por llamada (L, R, L, R…)
void TiraMatch::spawnNextLed() {
  int center     = spawnCenter;
  int leftStart  = center - 2;
  int rightStart = center + 2;

  if (spawnTurn == 0) {
    tone(BUZZER_PIN, 80, 25);   // click por cada LED de P1

    for (int i = 0; i < leftStart; i++)
      spawnBuffer[i] = spawnBuffer[i + 1];
    spawnBuffer[leftStart] = COLORS[leftColorIdx];

    if (++leftLedInSeg == SEGMENT_LEN) {
      leftLedInSeg = 0;
      leftColorIdx = pickOtherColor(leftColorIdx);
    }

    if (++leftSpawnCount % EROSION_INTERVAL == 0) erodeLeft();
  } else {
    for (int i = numLeds - 1; i > rightStart; i--)
      spawnBuffer[i] = spawnBuffer[i - 1];
    spawnBuffer[rightStart] = COLORS[rightColorIdx];

    if (++rightLedInSeg == SEGMENT_LEN) {
      rightLedInSeg = 0;
      rightColorIdx = pickOtherColor(rightColorIdx);
    }

    if (++rightSpawnCount % EROSION_INTERVAL == 0) erodeRight();
  }

  spawnTurn = 1 - spawnTurn;

  if (spawnIntervalMs > SPAWN_INTERVAL_MIN)
    spawnIntervalMs = max((float)SPAWN_INTERVAL_MIN, spawnIntervalMs - (float)SPAWN_ACCEL);
}

// Penalidad: escribe 8 LEDs directamente en el extremo más cercano al jugador del lado impactado
void TiraMatch::addPenalty(int player, CRGB color) {
  sendJoystickSoundToPlayer(player, SND_MISS);
  int center     = spawnCenter;
  int leftStart  = center - 2;
  int rightStart = center + 2;

  if (player == 1) {
    // Buscar el LED más externo (menor índice) del contenido del lado izquierdo
    int firstContent = -1;
    for (int i = 0; i <= leftStart; i++) {
      if (spawnBuffer[i] != CRGB::Black) { firstContent = i; break; }
    }
    // Escribir justo antes del contenido existente (o al inicio si no hay contenido)
    int endPos   = (firstContent < 0) ? min(SEGMENT_LEN - 1, leftStart) : firstContent - 1;
    int startPos = max(0, endPos - SEGMENT_LEN + 1);
    for (int i = startPos; i <= endPos; i++)
      spawnBuffer[i] = color;

  } else {
    // Buscar el LED más externo (mayor índice) del contenido del lado derecho
    int lastContent = -1;
    for (int i = numLeds - 1; i >= rightStart; i--) {
      if (spawnBuffer[i] != CRGB::Black) { lastContent = i; break; }
    }
    int startPos = (lastContent < 0) ? max(numLeds - SEGMENT_LEN, rightStart) : lastContent + 1;
    int endPos   = min(numLeds - 1, startPos + SEGMENT_LEN - 1);
    for (int i = startPos; i <= endPos; i++)
      spawnBuffer[i] = color;
  }
}

void TiraMatch::startShift(int dir, int steps) {
  shiftRemaining = steps;
  shiftDir       = dir;
  lastShift      = millis();
}

// Desplaza el buffer completo 1 LED y mueve el centro con él
void TiraMatch::stepShift() {
  if (shiftDir == 1) {
    for (int i = numLeds - 1; i > 0; i--)
      spawnBuffer[i] = spawnBuffer[i - 1];
    spawnBuffer[0] = CRGB::Black;
    spawnCenter = constrain(spawnCenter + 1, 4, numLeds - 5);
  } else {
    for (int i = 0; i < numLeds - 1; i++)
      spawnBuffer[i] = spawnBuffer[i + 1];
    spawnBuffer[numLeds - 1] = CRGB::Black;
    spawnCenter = constrain(spawnCenter - 1, 4, numLeds - 5);
  }
  shiftRemaining--;
}

void TiraMatch::erodeLeft() {
  int leftStart = spawnCenter - 2;
  for (int i = 0; i <= leftStart; i++) {
    if (spawnBuffer[i] != CRGB::Black) { spawnBuffer[i] = CRGB::Black; return; }
  }
}

void TiraMatch::erodeRight() {
  int rightStart = spawnCenter + 2;
  for (int i = numLeds - 1; i >= rightStart; i--) {
    if (spawnBuffer[i] != CRGB::Black) { spawnBuffer[i] = CRGB::Black; return; }
  }
}

bool TiraMatch::checkGameOver() {
  if (spawnBuffer[0] != CRGB::Black)           loser = 1;
  else if (spawnBuffer[numLeds-1] != CRGB::Black) loser = 2;
  else return false;

  gameOver = true;
  for (int i = 0; i < MAX_SHOTS; i++) shots[i].active = false;
  for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;

  goPhase      = GO_BLINK;
  goBlinkCount = 0;
  goBlinkOn    = true;
  goTimer      = millis();
  tone(BUZZER_PIN, 440, 200);
  return true;
}

void TiraMatch::renderFrame() {
  for (int i = 0; i < numLeds; i++) leds[i] = spawnBuffer[i];

  int  center = spawnCenter;
  CRGB dimP1  = COLORS[p1ColorIdx]; dimP1.nscale8(25);
  CRGB dimP2  = COLORS[p2ColorIdx]; dimP2.nscale8(25);

  for (int i = 0; i < center - 1; i++)
    if (leds[i] == CRGB::Black) leds[i] = dimP1;
  for (int i = center + 2; i < numLeds; i++)
    if (leds[i] == CRGB::Black) leds[i] = dimP2;

  for (int i = 0; i < MAX_SHOTS; i++) {
    if (!shots[i].active) continue;
    Shot& s = shots[i];
    for (int j = 0; j < SHOT_LEN; j++) {
      int idx = s.pos - s.dir * j;
      if (idx >= 0 && idx < numLeds)
        leds[idx] = s.color;
    }
  }

  renderParticles();
  renderSpawner();
  FastLED.show();
}

void TiraMatch::renderSpawner() {
  int center       = spawnCenter;
  leds[center - 1] = CRGB::White;
  leds[center]     = CRGB::Black;
  leds[center + 1] = CRGB::White;
}

void TiraMatch::updateIntro() {
  unsigned long now     = millis();
  unsigned long elapsed = now - introStart;

  float progress = constrain((float)elapsed / 3000.0f, 0.0f, 1.0f);
  int   half     = numLeds / 2;
  int   pos      = (int)(progress * half);
  int   leftPos  = pos;
  int   rightPos = numLeds - 1 - pos;

  // Actualizar tono cada 30ms (agudo → medio)
  if (now - lastIntroSound >= 30) {
    lastIntroSound = now;
    int freq = (int)(4000.0f - progress * (4000.0f - 600.0f));
    tone(BUZZER_PIN, freq);
  }

  // Render: solo los dos LEDs blancos
  fill_solid(leds, numLeds, CRGB::Black);
  leds[leftPos]  = CRGB::White;
  leds[rightPos] = CRGB::White;
  FastLED.show();

  // Fin del intro: los LEDs se tocan
  if (leftPos >= rightPos || elapsed >= 3000) {
    noTone(BUZZER_PIN);
    introActive = false;
    unsigned long t = millis();
    lastSpawn  = t;
    lastMove   = t;
    lastShift  = t;
  }
}

void TiraMatch::updateGameOver() {
  unsigned long now = millis();

  if (goPhase == GO_BLINK) {
    if (now - goTimer >= 250) {
      goTimer   = now;
      goBlinkOn = !goBlinkOn;
      if (goBlinkOn) {
        goBlinkCount++;
        tone(BUZZER_PIN, 440, 180);
        if (goBlinkCount >= 5) {
          goPhase     = GO_SWEEP;
          goSweepStep = 0;
          goTimer     = now;
        }
      }
    }
    renderBlinkFrame();
    return;
  }

  if (goPhase == GO_SWEEP) {
    if (now - goTimer >= 20) {
      goTimer = now;

      if (loser == 1) {
        for (int i = 0; i < numLeds - 1; i++)
          spawnBuffer[i] = spawnBuffer[i + 1];
        spawnBuffer[numLeds - 1] = CRGB::Black;
      } else {
        for (int i = numLeds - 1; i > 0; i--)
          spawnBuffer[i] = spawnBuffer[i - 1];
        spawnBuffer[0] = CRGB::Black;
      }
      goSweepStep++;

      int base = map(goSweepStep, 0, numLeds, 2000, 60);
      int freq = constrain(base + random(-40, 41), 40, 2200);
      tone(BUZZER_PIN, freq, 16);

      bool allBlack = true;
      for (int i = 0; i < numLeds; i++) {
        if (spawnBuffer[i] != CRGB::Black) { allBlack = false; break; }
      }
      if (allBlack || goSweepStep >= numLeds) {
        fill_solid(leds, numLeds, CRGB::Black);
        FastLED.show();
        renderGameOver();      // muestra OLED "GAME OVER"
        goPhase = GO_VICTORY;
        goTimer = now;
        return;
      }
    }
    for (int i = 0; i < numLeds; i++) leds[i] = spawnBuffer[i];
    FastLED.show();
    return;
  }

  if (goPhase == GO_VICTORY) {
    if (now - goTimer >= 30) {
      goTimer = now;

      int center   = numLeds / 2;
      int winStart = (loser == 1) ? center : 0;
      int winEnd   = (loser == 1) ? numLeds - 1 : center - 1;

      // Contar LEDs apagados en la mitad ganadora
      int count = 0;
      for (int i = winStart; i <= winEnd; i++)
        if (leds[i] == CRGB::Black) count++;

      if (count == 0) {
        goPhase = GO_VICTORY_WAIT;
        goTimer = now;
        return;
      }

      // Elegir uno aleatoriamente
      int skip   = random(count);
      int chosen = winStart;
      for (int i = winStart; i <= winEnd; i++) {
        if (leds[i] == CRGB::Black) {
          if (skip-- == 0) { chosen = i; break; }
        }
      }
      leds[chosen] = CRGB::White;

      // Frecuencia: centro = más agudo, extremo = más grave
      int dist = abs(chosen - center);
      int freq = map(dist, 0, center, 4000, 80);
      tone(BUZZER_PIN, freq, 25);

      FastLED.show();
    }
    return;
  }
  if (goPhase == GO_VICTORY_WAIT) {
    if (now - goTimer >= 3000) {
      goPhase = GO_VICTORY_OFF;
      goTimer = now;
    }
    return;
  }

  if (goPhase == GO_VICTORY_OFF) {
    if (now - goTimer >= 30) {
      goTimer = now;

      int center   = numLeds / 2;
      int winStart = (loser == 1) ? center : 0;
      int winEnd   = (loser == 1) ? numLeds - 1 : center - 1;

      // Contar LEDs encendidos en la mitad ganadora
      int count = 0;
      for (int i = winStart; i <= winEnd; i++)
        if (leds[i] != CRGB::Black) count++;

      if (count == 0) {
        goPhase = GO_DONE;
        return;
      }

      // Elegir uno encendido aleatoriamente y apagarlo
      int skip   = random(count);
      int chosen = winStart;
      for (int i = winStart; i <= winEnd; i++) {
        if (leds[i] != CRGB::Black) {
          if (skip-- == 0) { chosen = i; break; }
        }
      }
      leds[chosen] = CRGB::Black;
      FastLED.show();
    }
    return;
  }

  // GO_DONE: pantalla estática ya mostrada
}

void TiraMatch::renderBlinkFrame() {
  for (int i = 0; i < numLeds; i++) leds[i] = spawnBuffer[i];
  if (!goBlinkOn) {
    int lo = spawnCenter - 2;
    int hi = spawnCenter + 2;
    if (loser == 1) {
      for (int i = 0; i < lo; i++) leds[i] = CRGB::Black;
    } else {
      for (int i = hi; i < numLeds; i++) leds[i] = CRGB::Black;
    }
  }
  FastLED.show();
}

void TiraMatch::renderGameOver() {
  clearLeds();
  displayClear();
  displayText(0, 14, "GAME OVER", u8g2_font_ncenB10_tr);
  char buf[20];
  sprintf(buf, "Pierde P%d!", loser);
  displayText(0, 35, buf);
  displaySend();
}

void TiraMatch::updateDisplay() {
  const char* names[6] = { "R", "G", "B", "Y", "C", "M" };
  char buf[22];
  sprintf(buf, "P1=%-2s    P2=%-2s", names[p1ColorIdx], names[p2ColorIdx]);
  displayClear();
  displayText(0, 14, "TiraMatch", u8g2_font_ncenB10_tr);
  displayText(0, 35, buf);
  displaySend();
}

int TiraMatch::findFreeSlot() {
  for (int i = 0; i < MAX_SHOTS; i++)
    if (!shots[i].active) return i;
  return -1;
}

int TiraMatch::pickOtherColor(int current) {
  int others[6], j = 0;
  for (int i = 0; i < activeColors; i++)
    if (i != current) others[j++] = i;
  return others[random(j)];
}

int TiraMatch::nextColorInCycle(int current) {
  return (current + 1) % activeColors;
}
