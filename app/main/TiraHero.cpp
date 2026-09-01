#include "TiraHero.h"
#include <NonBlockingRtttl.h>  // rtttl::tone/noTone: mismo canal LEDC que usa SoundManager

// Ordenadas de más lenta a más rápida (según ms por nota por defecto: (240000/bpm)/d).
static const int SONG_COUNT = 11;
static const char* SONGS[SONG_COUNT] = {
  "Led Zepelin-Stairway to Heaven:o=5,d=8,b=63,b=63:a4,c,e,a,b,e,c,b,c6,e,c,c6,f#,d,a4,f#,e.,16c,a4,4e,c,a4,e,g4,a4,4a4",
  "Smurfs:o=5,d=4,b=200,b=200:2c6,f6.,8c6,d6,a#,2g,c6.,8a,f,a,2g,p,16g,16a,16a#,16b,2c6,f6.,8c6,d6,a#,2g,c6.,8a,a#,e,2f,p,16g,16a,16a#,16b,2c6,f6.,8c6,d6,a#,2g,c6.,8a,f,a,2g,p,16g,16a,16a#,16b,2c6,f6.,8c6,d6,a#,2g,c6.,8a,a#,e,2f.,1p",
  "Barbie Girl:o=5,d=8,b=125,b=125:g#,e,g#,c#6,4a,4p,f#,d#,f#,b,4g#,f#,e,4p,e,c#,4f#,4c#,4p,f#,e,4g#,4f#",
  "Bolero:o=5,d=16,b=80,b=80:c6,8c6,b,c6,d6,c6,b,a,8c6,c6,a,4c6,8c6,b,c6,a,g,e,f,2g,g,f,e,d,e,f,g,a,4g,4g,g,a,b,a,g,f,e,d,e,d,8c,8c,c,d,8e,8f,4d,2g",
  "Adams Family:o=5,d=8,b=160,b=160:c,4f,a,4f,c,4b4,2g,f,4e,g,4e,g4,4c,2f,c,4f,a,4f,c,4b4,2g,f,4e,c,4d,e,1f,c,d,e,f,1p,d,e,f#,g,1p,d,e,f#,g,4p,d,e,f#,g,4p,c,d,e,f",
  "Simpsons:o=5,d=8,b=160,b=160:c6.,4e6,4f#6,a6,4g6.,4e6,4c6,a,f#,f#,f#,2g,p,p,f#,f#,f#,g,4a#.,c6,c6,c6,4c6",
  "Star Wars:o=6,d=8,b=180,b=180:f5,f5,f5,2a#5.,2f.,d#,d,c,2a#.,4f.,d#,d,c,2a#.,4f.,d#,d,d#,2c,4p,f5,f5,f5,2a#5.,2f.,d#,d,c,2a#.,4f.,d#,d,c,2a#.,4f.,d#,d,d#,2c",
  "Super Man:o=6,d=8,b=180,b=180:g5,g5,g5,4c,c,2g,p,g,a.,16g,f,1g,p,g5,g5,g5,4c,c,2g,p,g,a.,16g,f,a,2g.,4p,c,c,c,2b.,4g.,c,c,c,2b.,4g.,c,c,c,b,a,b,2c7,c,c,c,c,c,2c.",
  "Halloween:o=5,d=8,b=180,b=180:d6,g,g,d6,g,g,d6,g,d#6,g,d6,g,g,d6,g,g,d6,g,d#6,g,c#6,f#,f#,c#6,f#,f#,c#6,f#,d6,f#,c#6,f#,f#,c#6,f#,f#,c#6,f#,d6,f#",
  "Final Countdown:o=5,d=16,b=125,b=125:b,a,4b,4e,4p,8p,c6,b,8c6,8b,4a,4p,8p,c6,b,4c6,4e,4p,8p,a,g,8a,8g,8f#,8a,4g.,f#,g,4a.,g,a,8b,8a,8g,8f#,4e,4c6,2b.,b,c6,b,a,1b",
  "Back to the Future:o=5,d=16,b=200,b=200:4g.,p,4c.,p,2f#.,p,g.,p,a.,p,8g,p,8e,p,8c,p,4f#,p,g.,p,a.,p,8g.,p,8d.,p,8g.,p,8d6.,p,4d6.,p,4c#6,p,b.,p,c#6.,p,2d6."
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
  travelMs  = (uint32_t)((float)(numLeds / 2 - TH_BASE_OFFSET) / speed);

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
  for (int i = 0; i < TH_MAX_SPARKS;  i++) sparks[i].flags = 0;
  songIdx = 0;
  parseRTTTL(SONGS[songIdx]);
  nextNote    = 0;
  songStartMs = millis();
  lastUpdate  = millis();
  inGap       = false;
  gapUntil    = 0;
  buzzerOffAt    = 0;
  melodyEndMs    = 0;
  lastMelodyFreq = 0;
  lastBeatNum    = -1;
  for (int p = 0; p < 2; p++) {
    buttonHeld[p]     = false;
    badHold[p]        = false;
    goodHold[p]       = false;
    activeHitBlock[p] = -1;
    penaltyCount[p]   = 0;
    playerOut[p]      = false;
  }
  gameOver = false;
  endMs    = 0;
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
  int dir    = (ni % 2 == 0) ? -1 : +1;   // par → P2 (desde extremo derecho), impar → P1 (desde extremo izquierdo)

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
  b.visible  = (n.size >= 2);  // semicorcheas y más largas
  b.missed   = false;
  b.hit      = false;

  if (dir == +1)
    b.leftEdge = -(float)(n.size - 1);  // P1: leading edge entra desde LED 0 (extremo izquierdo)
  else
    b.leftEdge = (float)numLeds;        // P2: leading edge entra desde LED 143 (extremo derecho)
}

static void killPlayerSparks(TH_Spark* sparks, int p) {
  for (int i = 0; i < TH_MAX_SPARKS; i++)
    if ((sparks[i].flags & 1) && sparks[i].player == (uint8_t)p) sparks[i].flags &= ~1u;
}

void TiraHero::addPenalty(int p) {
  if (playerOut[p]) return;
  penaltyCount[p]++;
  sendJoystickSoundToPlayer(p + 1, SND_ERROR);
  if (penaltyCount[p] >= 3) {
    playerOut[p] = true;
    killPlayerSparks(sparks, p);
    if (playerOut[0] && playerOut[1]) {
      gameOver = true;
      endMs    = millis() + 2000;
    }
  }
}

// ── Update ────────────────────────────────────────────────────────────
void TiraHero::update() {
  // Corta el tono actual manualmente en vez de usar tone(pin,freq,dur), y con
  // rtttl::noTone (no el global) para no pelearse con el canal LEDC que ya
  // reclamó la librería RTTTL — ver NonBlockingRtttl.h.
  unsigned long buzzerNow = millis();
  if (buzzerOffAt && buzzerNow >= buzzerOffAt) { rtttl::noTone(BUZZER_PIN); buzzerOffAt = 0; }

  if (gameOver) {
    if (millis() >= endMs) begin();
    else renderFrame();
    return;
  }

  unsigned long now = millis();
  if (now - lastUpdate < TH_UPDATE_MS) return;

  float dt   = (float)(now - lastUpdate);
  if (dt > 50.0f) dt = 50.0f;  // cap: evita saltos enormes si el loop se bloqueó
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
        ? (b.leftEdge + b.size - 1 >= (float)(numLeds / 2 - TH_BASE_OFFSET))  // P1 base en LED 71
        : (b.leftEdge <= (float)(numLeds / 2 + TH_BASE_OFFSET));               // P2 base en LED 73
      if (hit) {
        b.fired = true;
        // La nota siempre suena al llegar: el jugador tiene hasta offset LEDs para presionar
        if (b.freq > 0) {
          rtttl::tone(BUZZER_PIN, b.freq);
          buzzerOffAt    = now + b.durMs;
          melodyEndMs    = now + b.durMs;
          lastMelodyFreq = b.freq;
        }
      }
    }

    // Detectar miss: leading edge pasó base+offset sin que el player presionara
    // (fuera del if (!b.fired) para evaluarse en todos los frames post-fire)
    if (b.fired && b.visible && !b.missed && b.freq > 0) {
      int p = (b.dir == +1) ? 0 : 1;   // dir+1=P1=p0, dir-1=P2=p1
      if (!b.hit) {
        float leadEdge = (b.dir == +1) ? (b.leftEdge + b.size - 1) : b.leftEdge;
        float baseF    = (b.dir == +1) ? (float)(numLeds / 2 - TH_BASE_OFFSET) : (float)(numLeds / 2 + TH_BASE_OFFSET);
        float pastBase = (b.dir == +1) ? (leadEdge - baseF) : (baseF - leadEdge);
        if (pastBase > (float)TH_HIT_OFFSET) {
          b.missed = true;
          addPenalty(p);
        }
      }
    }

    // Desactivar al pasar la base o al salir de la tira
    bool pastBase = (b.dir == +1)
      ? (b.leftEdge > (float)(numLeds / 2 - TH_BASE_OFFSET + 2 * TH_HIT_OFFSET))
      : (b.leftEdge + b.size - 1 < (float)(numLeds / 2 + TH_BASE_OFFSET - 2 * TH_HIT_OFFSET));
    if (pastBase || b.leftEdge + b.size <= 0.0f || b.leftEdge >= (float)numLeds)
      b.active = false;
  }

  // Percusión: golpe grave en cada negra desde el inicio de la canción
  // (silenciada durante el gap entre canciones)
  if (!inGap) {
    int beatNum = (int)((float)(now - songStartMs) / quarterMs);
    if (beatNum > lastBeatNum) {
      lastBeatNum = beatNum;
      if (now >= melodyEndMs) {
        rtttl::tone(BUZZER_PIN, 65);
        buzzerOffAt = now + 70;
      }
    }
  }

  // Expirar goodHold cuando el bloque termina o el trailing edge pasó 8 LEDs la base
  // Solo penalizar (badHold) si el botón sigue apretado Y el hold fue demasiado prolongado;
  // si el bloque se desactivó naturalmente es fin correcto de nota → sin castigo
  for (int p = 0; p < 2; p++) {
    if (!goodHold[p] || activeHitBlock[p] < 0 || activeHitBlock[p] >= TH_MAX_BLOCKS) continue;
    TH_Block& b = blocks[activeHitBlock[p]];
    bool expired  = false;
    bool lateHold = false;
    if (!b.active) {
      expired = true;  // bloque terminó su recorrido → fin de nota, sin castigo
    } else {
      float trailEdge = (b.dir == +1) ? b.leftEdge : (b.leftEdge + b.size - 1);
      float baseF     = (b.dir == +1) ? (float)(numLeds / 2 - TH_BASE_OFFSET)
                                      : (float)(numLeds / 2 + TH_BASE_OFFSET);
      float pastTrail = (b.dir == +1) ? (trailEdge - baseF) : (baseF - trailEdge);
      if (pastTrail > 8.0f) { expired = true; lateHold = true; }
    }
    if (expired) {
      goodHold[p]       = false;
      activeHitBlock[p] = -1;
      if (lateHold && buttonHeld[p]) {  // hold prolongado mientras bloque activo → rojo
        killPlayerSparks(sparks, p);
        badHold[p] = true;
        addPenalty(p);
      }
    }
  }

  // Fin de canción: cura vidas una vez y arranca 2s de silencio antes de la siguiente.
  if (!inGap && nextNote >= noteCount) {
    bool any = false;
    for (int i = 0; i < TH_MAX_BLOCKS; i++)
      if (blocks[i].active) { any = true; break; }
    if (!any) {
      // Curar una vida a quien perdió alguna; revivir al eliminado con 1 vida (2 LEDs rojos)
      for (int p = 0; p < 2; p++) {
        if (playerOut[p]) {
          playerOut[p]    = false;
          penaltyCount[p] = 2;
        } else if (penaltyCount[p] > 0) {
          penaltyCount[p]--;
        }
      }
      inGap    = true;
      gapUntil = now + TH_SONG_GAP_MS;
    }
  }

  // Terminado el silencio: levanta el siguiente nivel de la lista de canciones.
  if (inGap && now >= gapUntil) {
    inGap       = false;
    songIdx     = (songIdx + 1) % SONG_COUNT;
    parseRTTTL(SONGS[songIdx]);
    nextNote    = 0;
    songStartMs = millis();
    lastBeatNum = -1;
  }

  // Chispas: spawn en la base mientras haya nota en la ventana
  for (int p = 0; p < 2; p++) {
    int   dir   = (p == 0) ? +1 : -1;
    float baseF = (p == 0) ? (float)(numLeds / 2 - TH_BASE_OFFSET)
                           : (float)(numLeds / 2 + TH_BASE_OFFSET);
    bool noteHere = false;
    for (int i = 0; i < TH_MAX_BLOCKS && !noteHere; i++) {
      TH_Block& b = blocks[i];
      if (!b.active || b.dir != dir || !b.fired || b.freq == 0 || !b.visible) continue;
      float leadEdge = (dir == +1) ? (b.leftEdge + b.size - 1) : b.leftEdge;
      float pastBase = (dir == +1) ? (leadEdge - baseF) : (baseF - leadEdge);
      if (pastBase > -(float)TH_HIT_OFFSET && pastBase < (float)(TH_HIT_OFFSET * 3))
        noteHere = true;
    }
    if (noteHere && (goodHold[p] || badHold[p])) {
      int sdir = (p == 0) ? +1 : -1;
      for (int i = 0; i < TH_MAX_SPARKS; i++) {
        if (!(sparks[i].flags & 1)) {
          float   vel  = speed * (1.5f + (float)random(0, 36) / 10.0f);
          uint8_t dist = (uint8_t)(4 + random(0, TH_BASE_OFFSET - 3));
          sparks[i] = { baseF, vel, dist, (int8_t)sdir, (uint8_t)p,
                        (uint8_t)(1u | (goodHold[p] ? 2u : 0u)) };
          break;
        }
      }
    }
  }

  float center = (float)(numLeds / 2);
  for (int i = 0; i < TH_MAX_SPARKS; i++) {
    TH_Spark& s = sparks[i];
    if (!(s.flags & 1)) continue;
    s.pos += (float)s.dir * s.vel * dt;
    float startPos = (s.player == 0) ? (float)(numLeds / 2 - TH_BASE_OFFSET)
                                     : (float)(numLeds / 2 + TH_BASE_OFFSET);
    float traveled = (s.dir == +1) ? (s.pos - startPos) : (startPos - s.pos);
    if (traveled >= (float)s.maxDist ||
        (s.dir == +1 && s.pos >= center) ||
        (s.dir == -1 && s.pos <= center))
      s.flags &= ~1u;
  }

  renderFrame();
}

// ── Render ────────────────────────────────────────────────────────────
void TiraHero::renderFrame() {
  if (gameOver) {
    fill_solid(leds, numLeds, CRGB::Red);
    showLeds();
    return;
  }
  clearLeds();

  // Player eliminado: fondo rojo desde el extremo hasta su base (no hasta el centro),
  // debajo de las notas, que van a seguir pasando "apagadas" por encima
  for (int p = 0; p < 2; p++) {
    if (!playerOut[p]) continue;
    if (p == 0) for (int j = 0; j <= numLeds / 2 - TH_BASE_OFFSET; j++)         setLed(j, CRGB::Red);
    else        for (int j = numLeds / 2 + TH_BASE_OFFSET; j < numLeds; j++)    setLed(j, CRGB::Red);
  }

  // Bloques (silencios no se pintan)
  for (int i = 0; i < TH_MAX_BLOCKS; i++) {
    TH_Block& b = blocks[i];
    if (!b.active || b.freq == 0 || !b.visible) continue;
    int left     = (int)b.leftEdge;
    int right    = left + b.size - 1;
    int leadEdge = (b.dir == +1) ? right : left;
    int center   = numLeds / 2;
    // Clipear cada bloque a su mitad del strip
    int p1base = numLeds / 2 - TH_BASE_OFFSET;  // LED 57
    int p2base = numLeds / 2 + TH_BASE_OFFSET;  // LED 87
    int clipL = (b.dir == +1) ? max(left, 0)           : max(left, p2base + 1);
    int clipR = (b.dir == +1) ? min(right, p1base - 1) : min(right, numLeds - 1);

    int bp = (b.dir == +1) ? 0 : 1;
    if (playerOut[bp]) {
      // Player eliminado: la nota sigue su recorrido pero apagada (negra) sobre el fondo rojo
      for (int j = clipL; j <= clipR; j++) setLed(j, CRGB::Black);
      continue;
    }

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

  for (int i = 0; i < TH_MAX_SPARKS; i++) {
    TH_Spark& s = sparks[i];
    if (!(s.flags & 1)) continue;
    int pos = (int)s.pos;
    if (pos < 0 || pos >= numLeds) continue;
    float startPos = (s.player == 0) ? (float)(numLeds / 2 - TH_BASE_OFFSET)
                                     : (float)(numLeds / 2 + TH_BASE_OFFSET);
    float traveled = (s.dir == +1) ? (s.pos - startPos) : (startPos - s.pos);
    float ratio    = 1.0f - traveled / (float)s.maxDist;
    uint8_t bright = (uint8_t)(255.0f * (ratio < 0.0f ? 0.0f : ratio));
    CRGB col = (s.flags & 2) ? CRGB::Green : CRGB::Red;
    col.nscale8(bright);
    setLed(pos, col);
  }

  // Indicadores de penalización: desde el centro hacia cada player
  int ctr = numLeds / 2;
  for (int p = 0; p < 2; p++) {
    int cnt = penaltyCount[p] < 3 ? penaltyCount[p] : 3;
    for (int e = 0; e < cnt; e++) {
      int led = (p == 0) ? (ctr - 1 - e) : (ctr + 1 + e);
      setLed(led, CRGB::Red);
    }
  }

  // Bases siempre encima de los bloques: verde=bien, rojo=error, blanco=idle
  auto getBaseColor = [&](int p) -> CRGB {
    if (badHold[p])  return CRGB::Red;
    if (goodHold[p]) return CRGB::Green;
    return CRGB::White;
  };
  if (!playerOut[0]) setLed(numLeds / 2 - TH_BASE_OFFSET, getBaseColor(0));
  if (!playerOut[1]) setLed(numLeds / 2 + TH_BASE_OFFSET, getBaseColor(1));

  showLeds();
}

// ── Input ─────────────────────────────────────────────────────────────
void TiraHero::onInput(int player, int button) {
  if (player < 1 || player > 2) return;
  int p    = player - 1;
  if (playerOut[p] || gameOver) return;
  buttonHeld[p] = true;
  int dir  = (player == 1) ? +1 : -1;
  float base = (player == 1) ? (float)(numLeds / 2 - TH_BASE_OFFSET) : (float)(numLeds / 2 + TH_BASE_OFFSET);

  // Buscar nota cuyo leading edge esté dentro del offset de la base
  for (int i = 0; i < TH_MAX_BLOCKS; i++) {
    TH_Block& b = blocks[i];
    if (!b.active || b.dir != dir) continue;
    float leadEdge = (dir == +1) ? (b.leftEdge + b.size - 1) : b.leftEdge;
    if (fabsf(leadEdge - base) <= (float)TH_HIT_OFFSET) {
      killPlayerSparks(sparks, p);
      goodHold[p]       = true;
      badHold[p]        = false;
      activeHitBlock[p] = i;
      b.hit             = true;
      return;
    }
  }
  // No había nota → solo rojo en base
  killPlayerSparks(sparks, p);
  badHold[p]        = true;
  goodHold[p]       = false;
  activeHitBlock[p] = -1;
}

void TiraHero::onButtonUp(int player, int button) {
  if (player < 1 || player > 2) return;
  int p = player - 1;
  if (playerOut[p] || gameOver) return;
  buttonHeld[p] = false;
  badHold[p]    = false;

  if (goodHold[p] && activeHitBlock[p] >= 0) {
    goodHold[p]       = false;
    activeHitBlock[p] = -1;
  }
}
