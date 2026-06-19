#include "TiraMatch.h"

const CRGB TiraMatch::COLORS[3] = { CRGB::Red, CRGB::Green, CRGB::Blue };

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
  leftColorIdx   = random(3);
  rightColorIdx  = random(3);

  shiftRemaining = 0;
  shiftDir       = 0;
  gameOver       = false;
  loser          = 0;

  unsigned long now = millis();
  lastMove  = now;
  lastSpawn = now;
  lastShift = now;

  clearLeds();
  updateDisplay();
  Serial.println("TiraMatch. 1/2=disparar  9/0=cambiar color");
}

void TiraMatch::update() {
  if (gameOver) { renderGameOver(); return; }

  if (Serial.available()) {
    char c = Serial.read();
    if (c == '1') onInput(1, 0);
    if (c == '2') onInput(2, 0);
    if (c == '9') onInput(1, 1);
    if (c == '0') onInput(2, 1);
  }

  unsigned long now = millis();

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
  if (gameOver) return;
  if (button == 0) {
    fire(player);
  } else {
    if (player == 1) p1ColorIdx = nextColorInCycle(p1ColorIdx);
    else             p2ColorIdx = nextColorInCycle(p2ColorIdx);
    updateDisplay();
  }
}

void TiraMatch::fire(int player) {
  int slot = findFreeSlot();
  if (slot < 0) return;
  playSound(SND_SHOOT);

  Shot& s  = shots[slot];
  s.active = true;

  if (player == 1) {
    s.pos   = SHOT_LEN - 1;
    s.dir   = 1;
    s.color = COLORS[p1ColorIdx];
  } else {
    s.pos   = numLeds - SHOT_LEN;
    s.dir   = -1;
    s.color = COLORS[p2ColorIdx];
  }
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
  int center     = numLeds / 2;
  int leftStart  = center - 2;
  int rightStart = center + 2;

  for (int i = 0; i < MAX_SHOTS; i++) {
    if (!shots[i].active) continue;
    int head = shots[i].pos;

    if (shots[i].dir == 1 && head <= leftStart) {
      if (spawnBuffer[head] != CRGB::Black) {
        if (spawnBuffer[head] == shots[i].color) {
          destroySegAt(head, 0, leftStart);
          startShift(1);
        } else {
          addPenalty(1, shots[i].color);
        }
        shots[i].active = false;
      }
    }

    if (shots[i].dir == -1 && head >= rightStart) {
      if (spawnBuffer[head] != CRGB::Black) {
        if (spawnBuffer[head] == shots[i].color) {
          destroySegAt(head, rightStart, numLeds - 1);
          startShift(-1);
        } else {
          addPenalty(2, shots[i].color);
        }
        shots[i].active = false;
      }
    }
  }
}

// Destruye hasta SEGMENT_LEN LEDs contiguos del mismo color alrededor de hitIdx
void TiraMatch::destroySegAt(int hitIdx, int minBound, int maxBound) {
  CRGB hitColor = spawnBuffer[hitIdx];
  int  start = hitIdx, end = hitIdx;

  while (start > minBound && spawnBuffer[start - 1] == hitColor) start--;
  while (end   < maxBound && spawnBuffer[end   + 1] == hitColor) end++;

  int count = min(end - start + 1, (int)SEGMENT_LEN);
  for (int i = start; i < start + count; i++)
    spawnBuffer[i] = CRGB::Black;
}

// Spawn alternado: un lado por llamada (L, R, L, R…)
void TiraMatch::spawnNextLed() {
  int center     = numLeds / 2;
  int leftStart  = center - 2;
  int rightStart = center + 2;

  if (spawnTurn == 0) {
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
    spawnIntervalMs = max(SPAWN_INTERVAL_MIN, spawnIntervalMs - SPAWN_ACCEL);
}

// Penalidad: empuja 8 LEDs del color del disparo errado hacia el extremo del jugador
void TiraMatch::addPenalty(int player, CRGB color) {
  int center     = numLeds / 2;
  int leftStart  = center - 2;
  int rightStart = center + 2;

  if (player == 1) {
    for (int k = 0; k < SEGMENT_LEN; k++) {
      for (int i = 0; i < leftStart; i++)
        spawnBuffer[i] = spawnBuffer[i + 1];
      spawnBuffer[leftStart] = color;
    }
  } else {
    for (int k = 0; k < SEGMENT_LEN; k++) {
      for (int i = numLeds - 1; i > rightStart; i--)
        spawnBuffer[i] = spawnBuffer[i - 1];
      spawnBuffer[rightStart] = color;
    }
  }
}

void TiraMatch::startShift(int dir) {
  shiftRemaining = SHIFT_STEPS;
  shiftDir       = dir;
  lastShift      = millis();
}

// Desplaza todo el buffer hacia el lado del oponente (1 LED por paso)
void TiraMatch::stepShift() {
  if (shiftDir == 1) {
    for (int i = numLeds - 1; i > 0; i--)
      spawnBuffer[i] = spawnBuffer[i - 1];
    spawnBuffer[0] = CRGB::Black;
  } else {
    for (int i = 0; i < numLeds - 1; i++)
      spawnBuffer[i] = spawnBuffer[i + 1];
    spawnBuffer[numLeds - 1] = CRGB::Black;
  }
  shiftRemaining--;
}

void TiraMatch::erodeLeft() {
  int leftStart = numLeds / 2 - 2;
  for (int i = 0; i <= leftStart; i++) {
    if (spawnBuffer[i] != CRGB::Black) { spawnBuffer[i] = CRGB::Black; return; }
  }
}

void TiraMatch::erodeRight() {
  int rightStart = numLeds / 2 + 2;
  for (int i = numLeds - 1; i >= rightStart; i--) {
    if (spawnBuffer[i] != CRGB::Black) { spawnBuffer[i] = CRGB::Black; return; }
  }
}

bool TiraMatch::checkGameOver() {
  if (spawnBuffer[0] != CRGB::Black)           { gameOver = true; loser = 1; renderGameOver(); return true; }
  if (spawnBuffer[numLeds - 1] != CRGB::Black) { gameOver = true; loser = 2; renderGameOver(); return true; }
  return false;
}

void TiraMatch::renderFrame() {
  for (int i = 0; i < numLeds; i++) leds[i] = spawnBuffer[i];

  int  center = numLeds / 2;
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

  renderSpawner();
  FastLED.show();
}

void TiraMatch::renderSpawner() {
  int center       = numLeds / 2;
  leds[center - 1] = CRGB::White;
  leds[center]     = CRGB::Black;
  leds[center + 1] = CRGB::White;
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
  const char* names[3] = { "R", "G", "B" };
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
  int others[2], j = 0;
  for (int i = 0; i < 3; i++)
    if (i != current) others[j++] = i;
  return others[random(2)];
}

int TiraMatch::nextColorInCycle(int current) {
  return (current + 1) % 3;
}
