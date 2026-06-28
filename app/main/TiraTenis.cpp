#include "TiraTenis.h"

TiraTenis::TiraTenis(CRGB* leds, int numLeds, U8G2* display)
  : GameBase(leds, numLeds, display) {}

void TiraTenis::begin() {
  netPos        = numLeds / 2;
  state         = TT_SERVING;
  servingPlayer = 0;
  serveStart    = millis();

  pos[0] = TT_P1_START;
  pos[1] = numLeds - 1 - TT_P2_START;
  vel[0] = 0.0f;
  vel[1] = 0.0f;

  ballPos      = pos[0] + 1.0f;
  ballVel      = 0.0f;
  hitSuccess   = false;
  hitStrength  = 0.0f;
  hitFromServe = true;
  lastBallLed  = (int)round(ballPos);
  sameledStart = millis();
  deadStart    = 0;

  for (int p = 0; p < 2; p++)
    for (int b = 0; b < 2; b++)
      held[p][b] = false;

  lastUpdate = millis();
  clearLeds();
}

// ── Easing de la pelota en saque ─────────────────────────────────────
// t=0→1: ease-out (rápido→lento), t=1→2: ease-in (lento→rápido)
float TiraTenis::serveBallFraction(unsigned long elapsed) {
  float halfPeriod = TT_SERVE_PERIOD_MS / 2.0f;
  float t = fmodf((float)elapsed / halfPeriod, 2.0f);
  if (t < 1.0f)
    return 2.0f * t - t * t;       // ease-out
  float s = t - 1.0f;
  return 1.0f - s * s;             // ease-in
}

// ── Movimiento genérico de un jugador ────────────────────────────────
static void movePlayer(float& p, float& v, bool* held, int dir, float minPos, float maxPos) {
  if (held[0] && !held[1]) {
    v += TT_ACCEL * dir;
  } else if (held[1] && !held[0]) {
    v -= TT_ACCEL * dir;
  } else {
    v *= TT_DECEL;
    if (fabsf(v) < 0.01f) v = 0.0f;
  }
  v = constrain(v, -TT_MAX_SPEED, TT_MAX_SPEED);
  p += v;
  if (p < minPos) { p = minPos; v = 0.0f; }
  if (p > maxPos) { p = maxPos; v = 0.0f; }
}

// ── Verifica hit y calcula fuerza ────────────────────────────────────
void TiraTenis::triggerHit(int p, bool fromServe) {
  float zoneNear = (p == 0) ? pos[p] + 1 : pos[p] - TT_HIT_LEN;
  float zoneFar  = (p == 0) ? pos[p] + TT_HIT_LEN : pos[p] - 1;

  hitSuccess   = (ballPos >= min(zoneNear, zoneFar) && ballPos <= max(zoneNear, zoneFar));
  hitStrength  = 0.0f;

  if (hitSuccess) {
    // distancia desde el LED más cercano al jugador (0=100%, TT_HIT_LEN-1=10%)
    float dist = (p == 0) ? ballPos - (pos[p] + 1) : (pos[p] - 1) - ballPos;
    dist = constrain(dist, 0.0f, (float)(TT_HIT_LEN - 1));
    hitStrength = 1.0f - (dist / (TT_HIT_LEN - 1)) * 0.9f;
  }

  hittingPlayer = p;
  hitFromServe  = fromServe;
  hitStart      = millis();
  state         = TT_HITTING;
}

// ── Estado: SAQUE ────────────────────────────────────────────────────
void TiraTenis::updateServe() {
  int sp = servingPlayer;
  int op = 1 - sp;

  // Solo el que NO saca puede moverse
  float minOp = (op == 0) ? 0.0f               : (float)(netPos + 1);
  float maxOp = (op == 0) ? (float)(netPos - 1) : (float)(numLeds - 1);
  movePlayer(pos[op], vel[op], held[op], (op == 0) ? 1 : -1, minOp, maxOp);
  vel[sp] = 0.0f;

  // Animación pelota
  float fraction = serveBallFraction(millis() - serveStart);
  if (sp == 0) {
    float ballStart = pos[0] + 1.0f;
    ballPos = ballStart + (pos[0] + TT_SERVE_TRAVEL - ballStart) * fraction;
  } else {
    float ballStart = pos[1] - 1.0f;
    ballPos = ballStart + (pos[1] - TT_SERVE_TRAVEL - ballStart) * fraction;
  }
}

// ── Estado: GOLPE ────────────────────────────────────────────────────
void TiraTenis::updateHitting() {
  // El que golpea está freezado; el otro puede moverse
  int op  = 1 - hittingPlayer;
  float minOp = (op == 0) ? 0.0f               : (float)(netPos + 1);
  float maxOp = (op == 0) ? (float)(netPos - 1) : (float)(numLeds - 1);
  movePlayer(pos[op], vel[op], held[op], (op == 0) ? 1 : -1, minOp, maxOp);
  vel[hittingPlayer] = 0.0f;

  if (millis() - hitStart < TT_HIT_MS) return;

  // Fin de la animación del golpe
  if (hitSuccess) {
    float speed = TT_BALL_MIN_SPEED + hitStrength * (TT_BALL_MAX_SPEED - TT_BALL_MIN_SPEED);
    ballVel = (hittingPlayer == 0) ? speed : -speed;
    state   = TT_RALLY;
  } else {
    // Fallo: vuelve al saque con la misma persona sacando
    if (hitFromServe) {
      serveStart = millis();
      state      = TT_SERVING;
    } else {
      // En rally: la pelota sigue su camino
      state = TT_RALLY;
    }
  }
}

// ── Estado: RALLY ────────────────────────────────────────────────────
void TiraTenis::updateRally() {
  // Mover jugadores: botón 0 = avanzar hacia red, botón 1 = retroceder
  for (int p = 0; p < 2; p++) {
    float minPos = (p == 0) ? 0.0f               : (float)(netPos + 1);
    float maxPos = (p == 0) ? (float)(netPos - 1) : (float)(numLeds - 1);
    movePlayer(pos[p], vel[p], held[p], (p == 0) ? 1 : -1, minPos, maxPos);
  }

  // Mover pelota con desaceleración suave
  ballVel *= TT_BALL_DECEL;
  ballPos += ballVel;

  // Pelota sale por los extremos → reset al saque
  if (ballPos < 0 || ballPos >= numLeds) {
    servingPlayer = (ballPos < 0) ? 1 : 0;
    serveStart    = millis();
    ballPos       = (servingPlayer == 0) ? pos[0] + 1.0f : pos[1] - 1.0f;
    ballVel       = 0.0f;
    lastBallLed   = (int)round(ballPos);
    sameledStart  = millis();
    state         = TT_SERVING;
    return;
  }

  // Detectar pelota quieta demasiado tiempo en el mismo LED
  int currentLed = (int)round(ballPos);
  if (currentLed != lastBallLed) {
    lastBallLed  = currentLed;
    sameledStart = millis();
  } else if (millis() - sameledStart > TT_BALL_STUCK_MS) {
    deadStart = millis();
    ballVel   = 0.0f;
    state     = TT_BALL_DEAD;
  }
}

// ── Estado: PELOTA MUERTA ────────────────────────────────────────────
void TiraTenis::updateDead() {
  if (millis() - deadStart < TT_DEAD_MS) return;

  // El jugador de la mitad donde está la pelota pierde → el contrario saca
  servingPlayer = (ballPos < netPos) ? 1 : 0;
  serveStart    = millis();
  ballPos       = (servingPlayer == 0) ? pos[0] + 1.0f : pos[1] - 1.0f;
  ballVel       = 0.0f;
  lastBallLed   = (int)round(ballPos);
  sameledStart  = millis();
  state         = TT_SERVING;
}

// ── Update principal ─────────────────────────────────────────────────
void TiraTenis::update() {
  unsigned long now = millis();
  if (now - lastUpdate < TT_UPDATE_MS) return;
  lastUpdate = now;

  if (state == TT_SERVING)
    updateServe();
  else if (state == TT_HITTING)
    updateHitting();
  else if (state == TT_RALLY)
    updateRally();
  else if (state == TT_BALL_DEAD)
    updateDead();

  renderFrame();
}

// ── Input ────────────────────────────────────────────────────────────
void TiraTenis::onInput(int player, int button) {
  if (player < 1 || player > 2) return;
  int p = player - 1;
  held[p][button] = true;

  if (state == TT_SERVING && p == servingPlayer) {
    triggerHit(p, true);
  }

  // Botón 0 en rally: hit si la pelota viene hacia el jugador y está a menos de TT_HIT_RANGE leds
  if (state == TT_RALLY && button == 0) {
    bool comingToward = (p == 0) ? (ballVel < 0) : (ballVel > 0);
    float dist        = fabsf(ballPos - pos[p]);
    if (comingToward && dist <= TT_HIT_RANGE) {
      triggerHit(p, false);
    }
    // si no cumple condiciones, held[p][0] ya está en true → movePlayer lo avanza
  }
}

void TiraTenis::onButtonUp(int player, int button) {
  if (player < 1 || player > 2) return;
  held[player - 1][button] = false;
}

// ── Render ───────────────────────────────────────────────────────────
void TiraTenis::renderFrame() {
  clearLeds();

  // Jugadores
  int p1 = (int)round(pos[0]);
  setLed(p1,     CRGB::White);
  setLed(p1 + 1, CRGB::White);

  int p2 = (int)round(pos[1]);
  setLed(p2 - 1, CRGB::White);
  setLed(p2,     CRGB::White);

  // Zona de golpe (encima de jugadores)
  if (state == TT_HITTING) {
    int base = (int)round(pos[hittingPlayer]);
    int dir  = (hittingPlayer == 0) ? 1 : -1;
    for (int i = 1; i <= TT_HIT_LEN; i++)
      setLed(base + dir * i, CRGB::Blue);
  }

  // Red (encima de todo excepto pelota)
  setLed(netPos, CRGB::White);

  // Pelota: roja si está muerta, amarilla en cualquier otro estado
  CRGB ballColor = (state == TT_BALL_DEAD) ? CRGB::Red : CRGB::Yellow;
  setLed((int)round(ballPos), ballColor);

  showLeds();
}
