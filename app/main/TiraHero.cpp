#include "TiraHero.h"

static const int SONG_COUNT = 2;
static const char* SONGS[SONG_COUNT] = {
  "Final Countdown:o=5,d=16,b=125:"
  "b,a,4b,4e,4p,8p,c6,b,8c6,8b,4a,4p,8p,c6,b,4c6,4e,4p,8p,"
  "a,g,8a,8g,8f#,8a,4g.,f#,g,4a.,g,a,8b,8a,8g,8f#,4e,4c6,2b.,"
  "b,c6,b,a,1b",
  "YMCA:o=5,d=8,b=160:"
  "c#6,a#,2p,a#,g#,f#,g#,a#,4c#6,a#,4c#6,d#6,a#,2p,a#,g#,f#,g#,a#,4c#6,a#,4c#6,"
  "d#6,b,2p,b,a#,g#,a#,b,4d#6,f#6,4d#6,4f6.,4d#6.,4c#6.,4b.,4a#,4g#"
};

// Frecuencias base octava 4: C C# D D# E F F# G G# A A# B
static const uint16_t BASE_FREQ[12] = {
  262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494
};

static uint16_t calcFreq(char n, bool sharp, int oct) {
  int idx;
  switch (n) {
    case 'c': idx=0;  break; case 'd': idx=2;  break; case 'e': idx=4; break;
    case 'f': idx=5;  break; case 'g': idx=7;  break; case 'a': idx=9; break;
    case 'b': idx=11; break; default: return 0;
  }
  if (sharp) idx++;
  uint32_t f = BASE_FREQ[idx % 12];
  for (int i = 4; i < oct; i++) f *= 2;
  for (int i = oct; i < 4; i++) f /= 2;
  return (uint16_t)f;
}

TiraHero::TiraHero(CRGB* leds, int numLeds, U8G2* display)
  : GameBase(leds, numLeds, display), noteCount(0), speed(0.01f), nextNote(0), lastUpdate(0) {}

// ── Parser RTTTL ─────────────────────────────────────────────────────
void TiraHero::parseRTTTL(const char* s) {
  noteCount = 0;

  // Saltar nombre
  while (*s && *s != ':') s++;
  if (*s) s++;

  // Defaults
  int defDur = 4, defOct = 6, bpm = 63;
  while (*s && *s != ':') {
    while (*s == ' ') s++;
    char key = *s++;
    if (*s == '=') s++;
    int v = 0;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    if      (key == 'd') defDur = v;
    else if (key == 'o') defOct = v;
    else if (key == 'b') bpm    = v;
    if (*s == ',') s++;
  }
  if (*s) s++;

  // ms por redonda = 4 × 60000 / bpm
  float msWhole   = 240000.0f / bpm;
  float msEighth  = msWhole / 8.0f;
  speed     = TH_LEDS_PER_EIGHTH / msEighth;
  quarterMs = msWhole / 4.0f;
  // LEDs que recorre el leading edge de un bloque derecho: centro(72)→base(143) = 71
  travelMs  = (uint32_t)((float)(numLeds / 2 - 1) / speed);

  uint32_t songMs = 0;

  while (*s && noteCount < TH_MAX_NOTES) {
    while (*s == ' ' || *s == ',') s++;
    if (!*s) break;

    // Duración
    int dur = 0;
    while (*s >= '0' && *s <= '9') dur = dur * 10 + (*s++ - '0');
    if (!dur) dur = defDur;

    // Nota
    char ch = 0; bool sharp = false, pause = false;
    if (*s == 'p' || *s == 'P') { pause = true; s++; }
    else { ch = *s++; if (*s == '#') { sharp = true; s++; } }

    // Octava
    int oct = defOct;
    if (*s >= '0' && *s <= '9') oct = *s++ - '0';

    // Puntillo
    bool dot = false;
    if (*s == '.') { dot = true; s++; }

    float noteMsF = msWhole / dur;
    if (dot) noteMsF *= 1.5f;

    // Tamaño en LEDs: corchea = 4, negra = 8, blanca = 16 ...
    int ledSize = (TH_LEDS_PER_EIGHTH * 8) / dur;
    if (dot) ledSize = ledSize * 3 / 2;
    if (ledSize < 1) ledSize = 1;

    notes[noteCount].freq    = pause ? 0 : calcFreq(ch, sharp, oct);
    notes[noteCount].size    = ledSize;
    notes[noteCount].spawnMs = songMs;
    notes[noteCount].durMs   = (uint32_t)noteMsF;
    noteCount++;
    songMs += (uint32_t)noteMsF;
  }
}

// ── Inicialización ────────────────────────────────────────────────────
void TiraHero::begin() {
  for (int i = 0; i < TH_MAX_BLOCKS; i++) blocks[i].active = false;
  songIdx = 0;
  parseRTTTL(SONGS[songIdx]);
  nextNote    = 0;
  songStartMs = millis();
  lastUpdate  = millis();
  melodyEndMs    = 0;
  lastMelodyFreq = 0;
  lastBeatNum    = -1;
  for (int p = 0; p < 2; p++) {
    badHold[p]        = false;
    goodHold[p]       = false;
    activeHitBlock[p] = -1;
  }
  clearLeds();
  showLeds();
}

int TiraHero::findFreeBlock() {
  for (int i = 0; i < TH_MAX_BLOCKS; i++)
    if (!blocks[i].active) return i;
  return -1;
}

// ── Spawn: notas alternadas — pares a P2 (derecha), impares a P1 (izquierda) ─
void TiraHero::spawnNote(int ni) {
  TH_Note& n   = notes[ni];
  int center = numLeds / 2;
  int dir    = (ni % 2 == 0) ? +1 : -1;   // par → P2 (derecha), impar → P1 (izquierda)

  // Amarillo y violeta alternados por nota dentro de cada player
  static const CRGB colA = CRGB(255, 180, 0);  // amarillo
  static const CRGB colB = CRGB(140, 0, 255);  // violeta
  int colorIdx = (ni / 2) % 2;  // 0,0,1,1,0,0... → cada player alterna A/B/A/B
  CRGB col = (n.freq == 0) ? CRGB::Black : (colorIdx == 0 ? colA : colB);

  int sl = findFreeBlock();
  if (sl < 0) return;
  TH_Block& b = blocks[sl];
  b.dir      = dir;
  b.size     = n.size;
  b.color    = col;
  b.freq     = n.freq;
  b.durMs    = n.durMs;
  b.active   = true;
  b.fired    = false;
  b.visible  = (n.size >= TH_LEDS_PER_EIGHTH);  // corcheas y más largas
  b.missed   = false;

  if (dir == +1)
    b.leftEdge = (float)(center - n.size + 1);    // leading edge (borde der) arranca en centro
  else
    b.leftEdge = (float)center;                   // leading edge (borde izq) arranca en centro
}

// ── Update ────────────────────────────────────────────────────────────
void TiraHero::update() {
  unsigned long now = millis();
  if (now - lastUpdate < TH_UPDATE_MS) return;

  float dt   = (float)(now - lastUpdate);
  lastUpdate = now;
  float move = speed * dt;

  // Spawear notas según timeline
  unsigned long elapsed = now - songStartMs;
  while (nextNote < noteCount && notes[nextNote].spawnMs <= elapsed)
    spawnNote(nextNote++);

  // Mover bloques y disparar sonido
  for (int i = 0; i < TH_MAX_BLOCKS; i++) {
    TH_Block& b = blocks[i];
    if (!b.active) continue;

    b.leftEdge += (float)b.dir * move;

    if (!b.fired) {
      bool hit = (b.dir == +1)
        ? (b.leftEdge + b.size - 1 >= (float)(numLeds - TH_BASE_SIZE))
        : (b.leftEdge <= (float)(TH_BASE_SIZE - 1));
      if (hit) {
        b.fired = true;
        // La nota siempre suena al llegar: el jugador tiene hasta offset LEDs para presionar
        if (b.freq > 0 && !badHold[0] && !badHold[1]) {
          tone(BUZZER_PIN, b.freq, b.durMs);
          melodyEndMs    = now + b.durMs;
          lastMelodyFreq = b.freq;
        }
      }
    }

    // Detectar miss: leading edge pasó base+offset sin que el player presionara
    // (fuera del if (!b.fired) para evaluarse en todos los frames post-fire)
    if (b.fired && b.visible && !b.missed && b.freq > 0) {
      int p = (b.dir == +1) ? 1 : 0;
      bool wasHit = (goodHold[p] && activeHitBlock[p] == i);
      if (!wasHit) {
        float leadEdge = (b.dir == +1) ? (b.leftEdge + b.size - 1) : b.leftEdge;
        float baseF    = (b.dir == +1) ? (float)(numLeds - 1) : 0.0f;
        float pastBase = (b.dir == +1) ? (leadEdge - baseF) : (baseF - leadEdge);
        if (pastBase > (float)TH_HIT_OFFSET) {
          b.missed = true;
          if (!badHold[0] && !badHold[1])
            tone(BUZZER_PIN, punishFreq(), 200);
        }
      }
    }

    // Desactivar cuando sale completamente de la tira
    if (b.leftEdge + b.size <= 0.0f || b.leftEdge >= (float)numLeds)
      b.active = false;
  }

  // Percusión: golpe grave en cada negra desde el inicio de la canción
  int beatNum = (int)((float)(now - songStartMs) / quarterMs);
  if (beatNum > lastBeatNum) {
    lastBeatNum = beatNum;
    if (now >= melodyEndMs && !badHold[0] && !badHold[1])
      tone(BUZZER_PIN, 65, 70);
  }

  // Detectar bloque held que salió del strip (sostenido demasiado tiempo)
  for (int p = 0; p < 2; p++) {
    if (goodHold[p] && activeHitBlock[p] >= 0 && !blocks[activeHitBlock[p]].active) {
      tone(BUZZER_PIN, punishFreq(), 250);
      goodHold[p]       = false;
      activeHitBlock[p] = -1;
    }
  }

  // Reiniciar cuando se acabó la melodía y no hay bloques activos
  if (nextNote >= noteCount) {
    bool any = false;
    for (int i = 0; i < TH_MAX_BLOCKS; i++)
      if (blocks[i].active) { any = true; break; }
    if (!any) {
      songIdx = (songIdx + 1) % SONG_COUNT;
      parseRTTTL(SONGS[songIdx]);
      nextNote    = 0;
      songStartMs = millis();
      lastBeatNum = -1;
    }
  }

  renderFrame();
}

// ── Render ────────────────────────────────────────────────────────────
void TiraHero::renderFrame() {
  clearLeds();

  // Bases de player: blanco normal, rojo si hay castigo activo
  setLed(0,           badHold[0] ? CRGB::Red : CRGB::White);
  setLed(numLeds - 1, badHold[1] ? CRGB::Red : CRGB::White);

  // Bloques (silencios no se pintan)
  for (int i = 0; i < TH_MAX_BLOCKS; i++) {
    TH_Block& b = blocks[i];
    if (!b.active || b.freq == 0 || !b.visible) continue;
    int left     = (int)b.leftEdge;
    int right    = left + b.size - 1;
    int leadEdge = (b.dir == +1) ? right : left;
    int center   = numLeds / 2;
    // Clipear cada bloque a su mitad del strip
    int clipL = (b.dir == +1) ? max(left, center)     : max(left, 0);
    int clipR = (b.dir == +1) ? min(right, numLeds-1) : min(right, center - 1);
    bool isGoodHit = (goodHold[0] && activeHitBlock[0] == i) ||
                     (goodHold[1] && activeHitBlock[1] == i);
    CRGB baseColor = b.missed    ? CRGB::Red
                   : isGoodHit  ? CRGB::Green
                   :              b.color;
    for (int j = clipL; j <= clipR; j++) {
      if (j == leadEdge) {
        setLed(j, baseColor);
      } else {
        CRGB dim = baseColor;
        dim.nscale8(15);   // ~6%
        setLed(j, dim);
      }
    }
  }

  // Centro siempre blanco (punto de spawn)
  setLed(numLeds / 2, CRGB::White);

  showLeds();
}

// ── Helpers ───────────────────────────────────────────────────────────
uint16_t TiraHero::punishFreq() {
  if (millis() < melodyEndMs && lastMelodyFreq > 0) {
    uint16_t f = lastMelodyFreq;
    while (f > 65) f >>= 1;           // bajar octavas hasta acercarse a 55 Hz
    return (f < 20) ? 20 : f;
  }
  return TH_PUNISH_FREQ;
}

// ── Input ─────────────────────────────────────────────────────────────
void TiraHero::onInput(int player, int button) {
  if (player < 1 || player > 2) return;
  int p    = player - 1;
  int dir  = (player == 2) ? +1 : -1;
  float base = (player == 2) ? (float)(numLeds - 1) : 0.0f;

  // Buscar nota cuyo leading edge esté dentro del offset de la base
  for (int i = 0; i < TH_MAX_BLOCKS; i++) {
    TH_Block& b = blocks[i];
    if (!b.active || b.dir != dir) continue;
    float leadEdge = (dir == +1) ? (b.leftEdge + b.size - 1) : b.leftEdge;
    if (fabsf(leadEdge - base) <= (float)TH_HIT_OFFSET) {
      goodHold[p]       = true;
      badHold[p]        = false;
      activeHitBlock[p] = i;
      return;
    }
  }
  // No había nota → castigo
  badHold[p]        = true;
  goodHold[p]       = false;
  activeHitBlock[p] = -1;
  tone(BUZZER_PIN, punishFreq());   // sin duración: suena hasta noTone()
}

void TiraHero::onButtonUp(int player, int button) {
  if (player < 1 || player > 2) return;
  int p = player - 1;

  if (badHold[p]) {
    noTone(BUZZER_PIN);
    badHold[p] = false;
    return;
  }

  if (goodHold[p] && activeHitBlock[p] >= 0) {
    TH_Block& b = blocks[activeHitBlock[p]];
    if (b.active) {
      float base       = (player == 2) ? (float)(numLeds - 1) : 0.0f;
      float trailEdge  = (b.dir == +1) ? b.leftEdge : (b.leftEdge + b.size - 1);
      if (fabsf(trailEdge - base) > (float)TH_HIT_OFFSET)
        tone(BUZZER_PIN, punishFreq(), 250);  // soltó fuera del offset
    }
    goodHold[p]       = false;
    activeHitBlock[p] = -1;
  }
}
