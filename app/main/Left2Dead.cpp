#include "Left2Dead.h"
#include <math.h>
#include <string.h>

extern bool isPlayerConnected(int player);  // definido en main.ino

// Patrones de crecimiento de un zombie, momento 1 a 10 (0=verde oscuro, G=verde, R=rojo)
static const char* const ENEMY_PATTERNS[L2D_ENEMY_STAGES] = {
  "0",
  "G",
  "0G0",
  "0GGG0",
  "0GRGRG0",
  "0GGRGGRGG0",
  "0GGGRGGGRGGG0",
  "0GGGRRGGGRRGGG0",
  "0GGGGRRGGGRRGGGG0",
  "0GGGGRRRGGGRRRGGGG0"
};

// greenColor: qué color mostrar en vez del verde normal (naranja/azul si algún
// player lo tiene en la mira, refuerza el "enganche"; verde normal si no). El
// '0' usa la versión oscura de ese mismo color (verde oscuro / naranja oscuro /
// azul oscuro), para que todo el zombie cambie de paleta junto, no solo el 'G'.
static CRGB enemyCharColor(char c, CRGB greenColor) {
  if (c == 'G') return greenColor;
  if (c == 'R') return CRGB::Red;
  CRGB dark = greenColor;
  dark.nscale8(60);
  return dark;  // '0'
}

Left2Dead::Left2Dead(CRGB* leds, int numLeds, U8G2* display)
  : GameBase(leds, numLeds, display) {}

void Left2Dead::begin() {
  float mid = numLeds / 2.0f;
  pos[0] = mid;
  pos[1] = mid;
  kills[0] = kills[1] = 0;
  lastFireTime[0] = lastFireTime[1] = 0;
  playerActive[0] = playerActive[1] = false;
  for (int i = 0; i < L2D_MAX_ENEMIES; i++) enemies[i].active = false;
  lastEnemySpawn       = millis();
  enemySpawnIntervalMs = (float)L2D_ENEMY_SPAWN_START_MS;
  nextSpawnSide        = 0;

  gameMode              = 1;
  modeStartMs           = millis();
  modeSpawningOff       = false;
  lastModeResetInterval = (float)L2D_ENEMY_SPAWN_START_MS;

  for (int i = 0; i < L2D_MAX_PARTICLES; i++) particles[i].active = false;
  lastParticleUpdate = millis();

  shotFlashCenter[0] = shotFlashCenter[1] = -1;

  alertActive[0]   = alertActive[1]   = false;
  lastAlertBeep[0] = lastAlertBeep[1] = 0;

  lastUpdate = millis();
  phase = L2D_WAITING;  // espera a que se conecte al menos un player (ver checkPlayers)

  clearLeds();
  updateDisplay();
  renderFrame();
}

// Activa a cada player en cuanto su joystick manda señal. El primero destraba
// L2D_WAITING; si se suma el segundo más tarde, empieza a dibujarse y a jugar
// también, sin interrumpir la partida en curso.
void Left2Dead::checkPlayers() {
  for (int p = 0; p < 2; p++) {
    if (playerActive[p] || !isPlayerConnected(p + 1)) continue;
    playerActive[p] = true;
    pos[p] = numLeds / 2.0f;
  }
}

// Posición absoluta: proyección de la inclinación (±45°) sobre toda la tira.
void Left2Dead::onAnalog(int player, int16_t /*tiltX*/, int16_t tiltY, int16_t /*tiltZ*/) {
  if (player < 1 || player > 2) return;
  int p = player - 1;

  float sinVal   = constrain(tiltY / 16384.0f, -1.0f, 1.0f);
  float angleDeg = asinf(sinVal) * 180.0f / PI;
  angleDeg = constrain(angleDeg, -L2D_ANGLE_MAX_DEG, L2D_ANGLE_MAX_DEG);
  float t  = (angleDeg + L2D_ANGLE_MAX_DEG) / (2.0f * L2D_ANGLE_MAX_DEG);  // 0..1

  float lo = 0.0f;
  float hi = (float)(numLeds - 1);
  float target = hi - t * (hi - lo);  // invertido: -45° => extremo hi, +45° => extremo lo
  pos[p] += (target - pos[p]) * L2D_POS_SMOOTHING;  // suaviza el salto entre LEDs vecinos
}

// Cualquiera de los 2 botones dispara
void Left2Dead::onInput(int player, int /*button*/) {
  if (player < 1 || player > 2) return;
  fire(player);
}

// ms por momento de crecimiento, según la dificultad actual (mismo cálculo
// que usa growEnemies(); factorizado acá para que enemyPos() lo pueda usar).
unsigned long Left2Dead::currentGrowMs() {
  float progress = (float)(L2D_ENEMY_SPAWN_START_MS - enemySpawnIntervalMs) /
                    (float)(L2D_ENEMY_SPAWN_START_MS - L2D_ENEMY_SPAWN_MIN_MS);
  return (unsigned long)(L2D_ENEMY_GROW_START_MS -
         progress * (L2D_ENEMY_GROW_START_MS - L2D_ENEMY_GROW_MIN_MS));
}

// Posición actual de un zombie: fija en modo 1; en modo 2 viaja de originLed a
// targetLed a medida que crece (momento 1 = recién sale, momento máximo = llegó
// y se pone rojo), con una curva ease-out (rápido al arrancar, lento al llegar).
// El progreso se interpola en tiempo real dentro del momento actual (no solo
// al cambiar de momento), para que el movimiento sea fluido cuadro a cuadro.
int Left2Dead::enemyPos(const L2D_Enemy& z) {
  if (!z.moving) return z.centerLed;

  unsigned long growMs = currentGrowMs();
  float stageProgress  = growMs > 0 ? (float)(millis() - z.lastGrow) / (float)growMs : 1.0f;
  stageProgress = constrain(stageProgress, 0.0f, 1.0f);

  float continuousStage = (float)(z.stage - 1) + stageProgress;
  float t = continuousStage / (float)(L2D_ENEMY_STAGES - 1);
  t = constrain(t, 0.0f, 1.0f);

  float eased = 1.0f - (1.0f - t) * (1.0f - t);
  return (int)roundf((float)z.originLed + (float)(z.targetLed - z.originLed) * eased);
}

// Si varios zombies se superponen en ese LED, gana siempre el más viejo (spawnTime menor).
int Left2Dead::findEnemyAt(int led) {
  int best = -1;
  for (int i = 0; i < L2D_MAX_ENEMIES; i++) {
    L2D_Enemy& z = enemies[i];
    if (!z.active) continue;
    int len   = (int)strlen(ENEMY_PATTERNS[z.stage - 1]);
    int center = enemyPos(z);
    int start = center - (len - 1) / 2;
    int end   = start + len - 1;
    if (led < start || led > end) continue;
    if (best < 0 || z.spawnTime < enemies[best].spawnTime) best = i;
  }
  return best;
}

// Beep de alerta en el control del player mientras su mira está enganchada a
// un enemigo: suena al engancharse y se repite cada L2D_ALERT_BEEP_MS hasta
// que lo mate o su mira salga de esa zona.
void Left2Dead::updateAlert() {
  unsigned long now = millis();
  for (int p = 0; p < 2; p++) {
    if (!playerActive[p]) continue;
    int c   = (int)roundf(pos[p]);
    int idx = findEnemyAt(c);

    if (idx < 0) {
      alertActive[p] = false;
      continue;
    }

    if (!alertActive[p] || now - lastAlertBeep[p] >= L2D_ALERT_BEEP_MS) {
      alertActive[p]   = true;
      lastAlertBeep[p] = now;
      sendJoystickSoundToPlayer(p + 1, SND_L2D_ALERT);
    }
  }
}

// El disparo es instantáneo: no hay bala que viaje ni que se dibuje, y nunca
// impacta al otro player. Solo afecta a un zombie si, en el mismo momento
// del disparo, hay uno exactamente en la posición del player. No consume
// munición, pero hay un cooldown mínimo entre disparos de un mismo player.
void Left2Dead::fire(int player) {
  if (phase != L2D_PLAYING) return;
  int p = player - 1;
  if (!playerActive[p]) return;

  unsigned long now = millis();
  if (now - lastFireTime[p] < L2D_FIRE_COOLDOWN_MS) return;
  lastFireTime[p] = now;

  int c = (int)roundf(pos[p]);
  shotFlashStart[p]  = now;
  shotFlashCenter[p] = c;

  int idx = findEnemyAt(c);
  if (idx >= 0) {
    L2D_Enemy& z = enemies[idx];
    int len = (int)strlen(ENEMY_PATTERNS[z.stage - 1]);
    spawnZombieExplosion(enemyPos(z), len);
    z.active = false;
    kills[p]++;
    playSound(SND_L2D_ZBOOM);  // reemplaza al sonido de disparo, no se superponen
    updateDisplay();
  } else {
    playSound(SND_L2D_SHOT);
  }
}

int Left2Dead::findFreeEnemy() {
  for (int i = 0; i < L2D_MAX_ENEMIES; i++)
    if (!enemies[i].active) return i;
  return -1;
}

// Cada spawn acelera el ritmo de aparición (3.0s → 2.8s → 2.6s → ... hasta 0.5s mínimo).
// Modo 1: posición fija al azar, alternando entre menor y mayor a la mitad de
// la tira. Modo 2: arranca en un extremo (alternando izquierda/derecha) y viaja
// hacia un punto al azar (ver enemyPos). Tras L2D_MODE_DURATION_MS de spawn
// activo, deja de aparecer gente nueva hasta que cambie de modo (ver
// checkModeTransition).
void Left2Dead::spawnEnemies() {
  unsigned long now = millis();

  if (!modeSpawningOff && now - modeStartMs >= L2D_MODE_DURATION_MS)
    modeSpawningOff = true;
  if (modeSpawningOff) return;

  // Con un solo player conectado, la mitad de ritmo de aparición (intervalo x2).
  bool solo = (playerActive[0] != playerActive[1]);
  unsigned long effectiveInterval = (unsigned long)enemySpawnIntervalMs * (solo ? L2D_SOLO_SPAWN_MULT : 1);
  if (now - lastEnemySpawn < effectiveInterval) return;
  lastEnemySpawn = now;

  int slot = findFreeEnemy();
  if (slot < 0) return;

  L2D_Enemy& e = enemies[slot];
  e.stage     = 1;
  e.lastGrow  = now;
  e.spawnTime = now;
  e.warning   = false;
  e.active    = true;

  if (gameMode == 1) {
    int mid = numLeds / 2;
    int center = (nextSpawnSide == 0)
      ? random(L2D_ENEMY_MIN_LED, mid)            // menor a la mitad
      : random(mid + 1, L2D_ENEMY_MAX_LED + 1);   // mayor a la mitad
    e.moving    = false;
    e.centerLed = center;
    e.originLed = center;
    e.targetLed = center;
  } else {
    int origin = (nextSpawnSide == 0) ? 0 : (numLeds - 1);   // desde cada extremo
    int target = random(L2D_ENEMY_MIN_LED, L2D_ENEMY_MAX_LED + 1);
    e.moving    = true;
    e.originLed = origin;
    e.targetLed = target;
    e.centerLed = origin;
  }
  nextSpawnSide = 1 - nextSpawnSide;

  enemySpawnIntervalMs = max((float)L2D_ENEMY_SPAWN_MIN_MS, enemySpawnIntervalMs - L2D_ENEMY_SPAWN_STEP_MS);
}

// Cuando se cumplió el tiempo de spawn del modo actual y no queda ningún
// zombie en pantalla, pasa al otro modo y reajusta la dificultad a mitad de
// camino entre el reset anterior y la actual (se va suavizando con cada salto).
void Left2Dead::checkModeTransition() {
  if (!modeSpawningOff) return;
  for (int i = 0; i < L2D_MAX_ENEMIES; i++)
    if (enemies[i].active) return;

  gameMode = (gameMode == 1) ? 2 : 1;

  float newInterval     = (lastModeResetInterval + enemySpawnIntervalMs) / 2.0f;
  lastModeResetInterval = newInterval;
  enemySpawnIntervalMs  = newInterval;

  modeStartMs     = millis();
  modeSpawningOff = false;
  nextSpawnSide   = 0;
}

// El crecimiento va relativo al ritmo de spawn actual: cuando los zombies
// empiezan a aparecer más rápido, también crecen más rápido. Al llegar al
// momento máximo entra en "advertencia" (todo rojo, fondo tenue, se sigue
// jugando); si pasan L2D_WARNING_MS sin ser eliminado, recién ahí ataca.
void Left2Dead::growEnemies() {
  unsigned long now    = millis();
  unsigned long growMs = currentGrowMs();

  for (int i = 0; i < L2D_MAX_ENEMIES; i++) {
    L2D_Enemy& z = enemies[i];
    if (!z.active) continue;

    if (z.stage < L2D_ENEMY_STAGES) {
      if (now - z.lastGrow < growMs) continue;
      z.lastGrow = now;
      z.stage++;
      if (z.stage >= L2D_ENEMY_STAGES) {
        z.warning      = true;
        z.warningUntil = now + L2D_WARNING_MS;
      }
      continue;
    }

    // Ya está en el momento máximo, en advertencia. Todavía dentro de la gracia: sigue jugándose.
    if (now < z.warningUntil) continue;

    // Se acabó la gracia sin ser eliminado: ataca.
    int len   = (int)strlen(ENEMY_PATTERNS[z.stage - 1]);
    int start = enemyPos(z) - (len - 1) / 2;
    z.active  = false;
    startFreeze(start, len);
    return;  // entra en la secuencia de fin de partida; deja de procesar el resto
  }
}

// El rojo opaco del zombie atacante arranca en su propio ancho y se disemina
// por toda la tira a lo largo de L2D_FREEZE_MS.
void Left2Dead::startFreeze(int start, int len) {
  phase           = L2D_FREEZE;
  phaseUntil      = millis() + L2D_FREEZE_MS;
  freezeStart     = start;
  freezeLen       = len;
  freezeAnimStart = millis();
  lastFreezeBeep  = 0;  // que la primera nota de la caída suene ya
  playSound(SND_L2D_HIT);
}

// Mientras se disemina el rojo, una nota al azar cada L2D_FREEZE_BEEP_MS,
// bajando de octava a medida que avanza la animación (secuencia descendente
// pero con notas al azar dentro de esa bajada).
void Left2Dead::updateFreezeSpread() {
  unsigned long now = millis();
  if (now - lastFreezeBeep < L2D_FREEZE_BEEP_MS) return;
  lastFreezeBeep = now;

  float progress = constrain((float)(now - freezeAnimStart) / (float)L2D_FREEZE_MS, 0.0f, 1.0f);
  int   octave   = 6 - (int)(progress * 4.0f);
  octave = constrain(octave, 2, 6);

  static const char NOTE_LETTERS[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b' };
  char note = NOTE_LETTERS[random(0, 7)];
  sprintf(freezeBeepBuf, "Fall:d=16,o=%d,b=300:%c", octave, note);
  playSound(freezeBeepBuf);
}

// Revela un LED por zombie matado, alternando P1/P2 desde cada extremo hacia
// el centro, hasta mostrar el conteo completo de ambos.
void Left2Dead::updateTally() {
  if (tallyShown[0] >= kills[0] && tallyShown[1] >= kills[1]) {
    phase      = L2D_TALLY_WAIT;
    phaseUntil = millis() + L2D_TALLY_END_WAIT_MS;
    return;
  }
  unsigned long now = millis();
  if (now - lastTallyStep < L2D_TALLY_STEP_MS) return;
  lastTallyStep = now;

  int turn = tallyTurn;
  if      (turn == 0 && tallyShown[0] >= kills[0]) turn = 1;
  else if (turn == 1 && tallyShown[1] >= kills[1]) turn = 0;

  tallyShown[turn]++;
  tallyTurn = 1 - turn;
}

// Explosión al matar un zombie: partículas verdes que saltan hacia ambos
// lados desde su posición, cada una con velocidad al azar; se frenan y se
// van apagando hasta desaparecer (igual mecánica que TiraMatch/TiraColors).
void Left2Dead::spawnZombieExplosion(int centerLed, int len) {
  int count = min(len, L2D_MAX_PARTICLES / 2);
  for (int i = 0; i < count; i++) {
    for (int d = -1; d <= 1; d += 2) {
      int slot = findFreeParticle();
      if (slot < 0) continue;
      L2D_Particle& p = particles[slot];
      p.active = true;
      p.pos    = (float)centerLed;
      p.vel    = random(225, 901) / 100.0f;  // 2.25 a 9.0 LEDs/frame
      p.maxVel = p.vel;
      p.dir    = d;
    }
  }
}

int Left2Dead::findFreeParticle() {
  for (int i = 0; i < L2D_MAX_PARTICLES; i++)
    if (!particles[i].active) return i;
  return -1;
}

void Left2Dead::updateParticles() {
  unsigned long now = millis();
  if (now - lastParticleUpdate < L2D_PARTICLE_MS) return;
  lastParticleUpdate = now;

  for (int i = 0; i < L2D_MAX_PARTICLES; i++) {
    L2D_Particle& p = particles[i];
    if (!p.active) continue;

    p.pos += p.vel * p.dir;
    p.vel *= L2D_PARTICLE_DECEL;

    if (p.vel < 0.1f || p.pos < 0 || p.pos >= numLeds) p.active = false;
  }
}

void Left2Dead::renderParticles() {
  for (int i = 0; i < L2D_MAX_PARTICLES; i++) {
    L2D_Particle& p = particles[i];
    if (!p.active) continue;
    int idx = (int)p.pos;
    if (idx < 0 || idx >= numLeds) continue;

    uint8_t brightness = (uint8_t)(p.vel / p.maxVel * 255.0f);
    setLed(idx, CRGB(0, brightness, 0));
  }
}

void Left2Dead::update() {
  unsigned long now = millis();
  if (now - lastUpdate < L2D_UPDATE_MS) return;
  lastUpdate = now;

  if (phase == L2D_WAITING) {
    checkPlayers();
    if (playerActive[0] || playerActive[1]) {
      phase          = L2D_PLAYING;
      lastEnemySpawn = now;   // no cuenta el tiempo de espera para el primer spawn
      modeStartMs    = now;
    }
    renderFrame();
    return;
  }

  if (phase == L2D_FREEZE) {
    updateFreezeSpread();
    if (millis() >= phaseUntil) {
      phase         = L2D_TALLY;
      tallyShown[0] = tallyShown[1] = 0;
      tallyTurn     = 0;
      lastTallyStep = millis() - L2D_TALLY_STEP_MS;  // que el primer LED aparezca ya
    }
    renderFrame();
    return;
  }

  if (phase == L2D_TALLY) {
    updateTally();
    renderFrame();
    return;
  }

  if (phase == L2D_TALLY_WAIT) {
    if (millis() >= phaseUntil) { begin(); return; }
    renderFrame();
    return;
  }

  checkPlayers();
  checkModeTransition();
  spawnEnemies();
  growEnemies();
  updateParticles();
  updateAlert();
  renderFrame();
}

void Left2Dead::updateDisplay() {
  char buf[22];
  sprintf(buf, "P1=%-2d    P2=%-2d", kills[0], kills[1]);
  displayClear();
  displayTitleBar("Left2Dead");
  displayText(0, 35, buf);
  displaySend();
}

void Left2Dead::renderFrame() {
  if (phase == L2D_WAITING) {
    // Esperando conexión: un pulso suave en el centro, para mostrar que sigue vivo.
    fill_solid(leds, numLeds, CRGB::Black);
    float phaseSin = sinf((float)millis() / 400.0f) * 0.5f + 0.5f;  // 0..1
    uint8_t bright = (uint8_t)(20 + phaseSin * 60);
    setLed(numLeds / 2, CRGB(bright, bright, bright));
    showLeds();
    return;
  }

  if (phase == L2D_FREEZE) {
    fill_solid(leds, numLeds, CRGB::Black);

    // El rojo opaco arranca en el ancho del zombie y se disemina hacia los
    // dos extremos de la tira, ease-in-out, hasta cubrirla entera.
    float progress = constrain((float)(millis() - freezeAnimStart) / (float)L2D_FREEZE_MS, 0.0f, 1.0f);
    float eased    = progress * progress * (3.0f - 2.0f * progress);

    int   zEnd      = freezeStart + freezeLen - 1;
    float leftDist  = (float)freezeStart;
    float rightDist = (float)((numLeds - 1) - zEnd);

    int lo = constrain(freezeStart - (int)roundf(eased * leftDist),  0, numLeds - 1);
    int hi = constrain(zEnd        + (int)roundf(eased * rightDist), 0, numLeds - 1);

    for (int i = lo; i <= hi; i++) setLed(i, CRGB::Red);
    showLeds();
    return;
  }

  if (phase == L2D_TALLY || phase == L2D_TALLY_WAIT) {
    fill_solid(leds, numLeds, CRGB::Black);
    for (int i = 0; i < tallyShown[0]; i++) setLed(i, CRGB::White);
    for (int i = 0; i < tallyShown[1]; i++) setLed(numLeds - 1 - i, CRGB::White);
    showLeds();
    return;
  }

  fill_solid(leds, numLeds, CRGB::Black);

  // Si algún zombie está en advertencia (llegó al momento máximo y todavía no
  // atacó), el fondo entero queda con un rojo muy tenue como aviso.
  bool anyWarning = false;
  for (int i = 0; i < L2D_MAX_ENEMIES; i++)
    if (enemies[i].active && enemies[i].warning) { anyWarning = true; break; }
  if (anyWarning) {
    CRGB bg = CRGB(L2D_WARNING_BG_R, 0, 0);
    for (int i = 0; i < numLeds; i++) setLed(i, bg);
  }

  // Enemigo que tiene en la mira cada player ahora mismo (-1 = ninguno o inactivo).
  int aimedEnemy[2] = { -1, -1 };
  if (playerActive[0]) aimedEnemy[0] = findEnemyAt((int)roundf(pos[0]));
  if (playerActive[1]) aimedEnemy[1] = findEnemyAt((int)roundf(pos[1]));

  // Zombies: se dibujan de más nuevo a más viejo, así el más viejo queda
  // "arriba" (pisa a los más nuevos) cuando dos se superponen en la tira.
  // Uno en advertencia se pinta sólido rojo en vez de su patrón de colores.
  // Si algún player lo tiene enganchado, sus LEDs verdes cambian de color
  // (naranja para el player amarillo, azul para el cian) para reforzar el enganche.
  {
    int order[L2D_MAX_ENEMIES];
    int n = 0;
    for (int i = 0; i < L2D_MAX_ENEMIES; i++)
      if (enemies[i].active) order[n++] = i;
    for (int a = 0; a < n; a++)
      for (int b = a + 1; b < n; b++)
        if (enemies[order[b]].spawnTime > enemies[order[a]].spawnTime) {
          int t = order[a]; order[a] = order[b]; order[b] = t;
        }

    for (int k = 0; k < n; k++) {
      int enemyIdx = order[k];
      L2D_Enemy& z = enemies[enemyIdx];

      CRGB greenColor = CRGB(0, 255, 0);
      if      (aimedEnemy[0] == enemyIdx) greenColor = CRGB::Orange;
      else if (aimedEnemy[1] == enemyIdx) greenColor = CRGB::Blue;

      const char* pattern = ENEMY_PATTERNS[z.stage - 1];
      int len   = (int)strlen(pattern);
      int start = enemyPos(z) - (len - 1) / 2;
      for (int j = 0; j < len; j++) {
        int led = start + j;
        if (led < 0 || led >= numLeds) continue;
        setLed(led, z.warning ? CRGB::Red : enemyCharColor(pattern[j], greenColor));
      }
    }
  }

  renderParticles();

  // Mira de cada player: un único LED central, en su color (P1 amarillo, P2 cian).
  // Si el centro cae dentro de un zombie, la mira se "engancha" a sus extremos
  // (blanco 100% opaco, siguiéndolo mientras crece) y el centro pasa a dibujarse
  // negro, siempre siguiendo la inclinación, hasta que salga de esa zona.
  const CRGB playerColor[2] = { CRGB::Yellow, CRGB(0, 255, 255) };
  for (int p = 0; p < 2; p++) {
    if (!playerActive[p]) continue;
    int c   = (int)roundf(pos[p]);
    int idx = aimedEnemy[p];

    if (idx >= 0) {
      L2D_Enemy& z = enemies[idx];
      int len   = (int)strlen(ENEMY_PATTERNS[z.stage - 1]);
      int start = enemyPos(z) - (len - 1) / 2;
      int end   = start + len - 1;
      setLed(constrain(start, 0, numLeds - 1), CRGB::White);
      setLed(constrain(end,   0, numLeds - 1), CRGB::White);
      setLed(constrain(c,     0, numLeds - 1), CRGB::Black);
    } else {
      setLed(constrain(c, 0, numLeds - 1), playerColor[p]);
    }
  }

  // Flash blanco al disparar: 5 LEDs (0.1s) → 3 LEDs (0.1s) → 1 LED (0.2s), centrado en la mira
  for (int p = 0; p < 2; p++) {
    if (shotFlashCenter[p] < 0) continue;
    unsigned long elapsed = millis() - shotFlashStart[p];

    int half;
    if      (elapsed < L2D_SHOT_FLASH_S1_MS) half = 2;  // 5 LEDs
    else if (elapsed < L2D_SHOT_FLASH_S2_MS) half = 1;  // 3 LEDs
    else if (elapsed < L2D_SHOT_FLASH_S3_MS) half = 0;  // 1 LED
    else { shotFlashCenter[p] = -1; continue; }

    int fc = shotFlashCenter[p];
    for (int j = -half; j <= half; j++) {
      int led = fc + j;
      if (led >= 0 && led < numLeds) setLed(led, CRGB::White);
    }
  }

  showLeds();
}
