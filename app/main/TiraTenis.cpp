#include "TiraTenis.h"

TiraTenis::TiraTenis(CRGB* leds, int numLeds, U8G2* display)
  : GameBase(leds, numLeds, display) {}

// ── Ease-in: arranca lento y acelera hasta el final del segmento ─────
static float easeInOut(float t) {
  return 1.0f - cosf(t * PI / 2.0f);
}

void TiraTenis::begin() {
  netPos        = numLeds / 2;
  state         = TT_SERVING;
  servingPlayer = 0;

  pos[0] = TT_SERVE_POS;
  pos[1] = numLeds - 1 - TT_SERVE_POS;
  vel[0] = vel[1] = 0.0f;

  serveStart  = millis();
  ballPos     = pos[0] + 1.0f;
  shadowPos   = ballPos;
  ballDir     = 1;
  lastHitter  = 0;
  hitSuccess      = false;
  hitStrength     = 0.0f;
  hitFromServe    = true;
  hitAnimStep     = 0;
  hitAnimPhase    = 0;
  hitAnimStepTime = 0;
  bounceMarkPos   = 0.0f;
  bounceMarkTime  = 0;
  deadStart       = 0;
  deadBallVel     = 0.0f;
  deadEraseCount  = -1;
  deadEraseTimer  = 0;

  tilt[0] = tilt[1] = 0.0f;
  hitAnimActive = false;
  for (int p = 0; p < 2; p++)
    playerFrozenUntil[p] = 0;

  introActive     = true;
  introPhase      = 0;
  introStart      = millis();
  introPhaseStart = millis();

  lastUpdate = millis();
  clearLeds();
}

// ── Easing saque ─────────────────────────────────────────────────────
float TiraTenis::serveBallFraction(unsigned long elapsed) {
  float halfPeriod = TT_SERVE_PERIOD_MS / 2.0f;
  float t = fmodf((float)elapsed / halfPeriod, 2.0f);
  if (t < 1.0f) return 2.0f * t - t * t;
  float s = t - 1.0f;
  return 1.0f - s * s;
}

// ── Movimiento genérico de jugador ────────────────────────────────────
static void movePlayer(float& p, float& v, float tilt, float minPos, float maxPos) {
  if (fabsf(tilt) < 0.05f) {
    v *= TT_DECEL;
    if (fabsf(v) < 0.01f) v = 0.0f;
  } else {
    v += tilt * TT_ACCEL;
    v = constrain(v, -TT_MAX_SPEED, TT_MAX_SPEED);
  }
  p += v;
  if (p < minPos) { p = minPos; v = 0.0f; }
  if (p > maxPos) { p = maxPos; v = 0.0f; }
}

// ── Inicia un segmento de trayectoria ─────────────────────────────────
void TiraTenis::startSegment(int seg) {
  ballSeg = seg;
  if (seg == 0) {
    segStartPos = pendingBallStart;
    segLen      = pendingTotalLeds * 2.0f / 3.0f;
    segMs       = pendingTotalMs   * 2.0f / 3.0f;
  } else {
    segStartPos = pendingBallStart + ballDir * pendingTotalLeds * 2.0f / 3.0f;
    segLen      = pendingTotalLeds / 3.0f;
    segMs       = (pendingTotalMs  / 3.0f) / 0.5f;  // 50% de velocidad post-pique
  }
  segStartTime    = millis();
  // Sombra: arranca 1 LED adelantado hacia el centro; llega al mismo punto que la pelota
  shadowSegStart  = segStartPos + ballDir;
}

// ── Hit: calcula trayectoria e inicia animación de barrido ────────────
void TiraTenis::triggerHit(int p, bool fromServe, bool lob) {
  float zoneNear = (p == 0) ? pos[p] + 1 : pos[p] - TT_HIT_LEN;
  float zoneFar  = (p == 0) ? pos[p] + TT_HIT_LEN : pos[p] - 1;

  hitSuccess  = (ballPos >= min(zoneNear, zoneFar) && ballPos <= max(zoneNear, zoneFar));
  hitStrength = 0.0f;

  if (hitSuccess) {
    float dist = (p == 0) ? ballPos - (pos[p] + 1) : (pos[p] - 1) - ballPos;
    dist        = constrain(dist, 0.0f, (float)(TT_HIT_LEN - 1));
    hitStrength = 1.0f - (dist / (TT_HIT_LEN - 1)) * 0.9f;
    float lerp  = (hitStrength - 0.1f) / 0.9f;
    float ledsMin, ledsMax, msMin, msMax;
    if (fromServe) {
      ledsMin = TT_SERVE_LEDS_MIN; ledsMax = TT_SERVE_LEDS_MAX;
      msMin   = TT_BALL_TIME_MIN_MS; msMax   = TT_BALL_TIME_MAX_MS;
    } else if (lob) {
      ledsMin = TT_LOB_LEDS_MIN; ledsMax = TT_LOB_LEDS_MAX;
      msMin   = TT_LOB_TIME_MIN_MS; msMax   = TT_LOB_TIME_MAX_MS;
    } else {
      ledsMin = TT_BALL_LEDS_MIN; ledsMax = TT_BALL_LEDS_MAX;
      msMin   = TT_BALL_TIME_MIN_MS; msMax   = TT_BALL_TIME_MAX_MS;
    }
    pendingTotalLeds = ledsMin + lerp * (ledsMax - ledsMin);
    pendingTotalMs   = msMin   + lerp * (msMax   - msMin);
    isLob            = lob;
    pendingBallStart = ballPos;
    ballDir          = (p == 0) ? 1 : -1;
    lastHitter       = p;
  }

  hittingPlayer   = p;
  hitFromServe    = fromServe;
  hitIsLob        = lob;
  hitStart        = millis();
  hitAnimStep     = 0;
  hitAnimPhase    = 0;
  hitAnimStepTime = millis();
  state           = TT_HITTING;
}

// ── Intro ─────────────────────────────────────────────────────────────
void TiraTenis::updateIntro() {
  unsigned long now = millis();

  if (introPhase == 0) {
    unsigned long elapsed = now - introStart;
    float progress = constrain((float)elapsed / 2000.0f, 0.0f, 1.0f);

    int leftLed  = (int)(progress * netPos);
    int rightLed = numLeds - 1 - leftLed;

    tone(BUZZER_PIN, (int)(4000.0f - progress * 3400.0f));

    fill_solid(leds, numLeds, CRGB::Black);
    if (leftLed < rightLed) {
      leds[leftLed]  = CRGB::White;
      leds[rightLed] = CRGB::White;
    } else {
      leds[netPos] = CRGB(40, 40, 40);
    }
    FastLED.show();

    if (leftLed >= rightLed || elapsed >= 2200) {
      noTone(BUZZER_PIN);
      introPhase      = 1;
      introPhaseStart = now;
      pos[0] = 0.0f;
      pos[1] = (float)(numLeds - 1);
    }

  } else {
    unsigned long elapsed = now - introPhaseStart;
    float progress = constrain((float)elapsed / 1000.0f, 0.0f, 1.0f);
    float t = progress * progress * (3.0f - 2.0f * progress);  // smoothstep

    pos[0] = t * TT_SERVE_POS;
    pos[1] = (numLeds - 1) - t * TT_SERVE_POS;

    int lp0 = (int)round(pos[0]);
    int lp1 = (int)round(pos[1]);

    fill_solid(leds, numLeds, CRGB::Black);
    leds[constrain(lp0,     0, numLeds - 1)] = CRGB::White;
    leds[constrain(lp0 + 1, 0, numLeds - 1)] = CRGB::White;
    leds[constrain(lp1 - 1, 0, numLeds - 1)] = CRGB::White;
    leds[constrain(lp1,     0, numLeds - 1)] = CRGB::White;
    leds[netPos] = CRGB(40, 40, 40);
    FastLED.show();

    if (progress >= 1.0f) {
      introActive = false;
      pos[0] = TT_SERVE_POS;
      pos[1] = numLeds - 1 - TT_SERVE_POS;
      vel[0] = vel[1] = 0.0f;
      lastUpdate = millis();
    }
  }
}

// ── Estado: SAQUE ─────────────────────────────────────────────────────
void TiraTenis::updateServe() {
  int sp = servingPlayer, op = 1 - sp;
  float minOp = (op == 0) ? 0.0f               : (float)(netPos + 1);
  float maxOp = (op == 0) ? (float)(netPos - 1) : (float)(numLeds - 1);
  movePlayer(pos[op], vel[op], tilt[op], minOp, maxOp);
  vel[sp] = 0.0f;

  float fraction = serveBallFraction(millis() - serveStart);
  if (sp == 0) {
    ballPos = pos[0] + 1.0f + (TT_SERVE_TRAVEL - 1.0f) * fraction;
  } else {
    ballPos = pos[1] - 1.0f - (TT_SERVE_TRAVEL - 1.0f) * fraction;
  }
}

// ── Estado: GOLPE (barrido azul→blanco, luego blanco→apagado) ────────
void TiraTenis::updateHitting() {
  int op = 1 - hittingPlayer;
  float minOp = (op == 0) ? 0.0f               : (float)(netPos + 1);
  float maxOp = (op == 0) ? (float)(netPos - 1) : (float)(numLeds - 1);
  movePlayer(pos[op], vel[op], tilt[op], minOp, maxOp);
  vel[hittingPlayer] = 0.0f;

  const unsigned long stepMs = TT_HIT_MS / TT_HIT_LEN;
  if (millis() - hitAnimStepTime < stepMs) return;
  hitAnimStepTime = millis();

  int dir  = (hittingPlayer == 0) ? 1 : -1;
  int base = (int)round(pos[hittingPlayer]);

  if (hitAnimPhase == 0) {
    // Avance: player → centro, azul → blanco
    hitAnimStep++;
    int sweepLed = base + dir * hitAnimStep;

    if (hitSuccess && sweepLed == (int)round(ballPos)) {
      // Barrido toca la pelota → cancela freeze general, reanuda el rally
      // El player queda frozen el tiempo restante de la animación completa (ida+vuelta)
      unsigned long remainingSteps = (TT_HIT_LEN - hitAnimStep) + TT_HIT_LEN;
      playerFrozenUntil[hittingPlayer] = millis() + remainingSteps * stepMs;
      sendJoystickSoundToPlayer(hittingPlayer + 1, hitIsLob ? SND_LOB : SND_HIT);
      startSegment(0);
      rallyStartTime = millis();
      state = TT_RALLY;
      return;
    }

    if (hitAnimStep >= TT_HIT_LEN)
      hitAnimPhase = 1;  // forward completo sin éxito → retroceso

  } else {
    // Retroceso: centro → player, blanco → apagado
    hitAnimStep--;
    if (hitAnimStep <= 0) {
      // Animación completa → miss, reanuda sin freeze
      if (hitFromServe) {
        serveStart = millis();
        state      = TT_SERVING;
      } else {
        segStartTime += millis() - hitStart;
        state = TT_RALLY;
      }
    }
  }
}

// ── Estado: RALLY ─────────────────────────────────────────────────────
void TiraTenis::updateRally() {
  unsigned long now = millis();
  for (int p = 0; p < 2; p++) {
    if (now < playerFrozenUntil[p]) { vel[p] = 0.0f; continue; }
    float minPos = (p == 0) ? 0.0f               : (float)(netPos + 1);
    float maxPos = (p == 0) ? (float)(netPos - 1) : (float)(numLeds - 1);
    movePlayer(pos[p], vel[p], tilt[p], minPos, maxPos);
  }

  float elapsed = (float)(millis() - segStartTime);
  float t       = constrain(elapsed / segMs, 0.0f, 1.0f);

  ballPos   = segStartPos + ballDir * segLen * easeInOut(t);
  shadowPos = shadowSegStart + t * (segStartPos + ballDir * segLen - shadowSegStart);

  // Calcula velocidad actual de la pelota para la animación de game over
  if (hitAnimActive) {
    const unsigned long stepMs = TT_HIT_MS / TT_HIT_LEN;
    if (millis() - hitAnimStepTime >= stepMs) {
      hitAnimStepTime = millis();
      if (hitAnimPhase == 0) {
        if (++hitAnimStep >= TT_HIT_LEN) hitAnimPhase = 1;
      } else {
        if (--hitAnimStep <= 0) hitAnimActive = false;
      }
    }
  }

  auto enterDead = [&](int loser) {
    loserPlayer   = loser;
    hitAnimActive = false;
    ballPos     = constrain(ballPos, 0.0f, (float)(numLeds - 1));
    float speed = sinf(t * PI / 2.0f) * (PI / 2.0f) * segLen / segMs;
    deadBallVel = (float)ballDir * speed * 0.5f;
    deadEraseCount = -1;
    deadStart      = millis();
    state          = TT_BALL_DEAD;
    playSound(SND_GAMEOVER);
  };

  if (ballPos < 0 || ballPos >= numLeds) {
    enterDead((ballSeg == 0) ? lastHitter : 1 - lastHitter);
    return;
  }

  if (t >= 1.0f) {
    if (ballSeg == 0) {
      bool piqueOwnHalf = (lastHitter == 0) ? (ballPos < netPos) : (ballPos >= netPos);
      if (piqueOwnHalf) {
        enterDead(lastHitter);
      } else {
        bounceMarkPos  = ballPos;
        bounceMarkTime = millis();
        startSegment(1);
      }
    } else {
      enterDead(1 - lastHitter);
    }
  }
}

static const char* const kDeadNotes[] = {
  "n:d=32,o=4,b=900:c", "n:d=32,o=4,b=900:d", "n:d=32,o=4,b=900:e",
  "n:d=32,o=4,b=900:g", "n:d=32,o=5,b=900:c", "n:d=32,o=3,b=900:a"
};

// ── Estado: PELOTA MUERTA ─────────────────────────────────────────────
void TiraTenis::updateDead() {
  // Pelota sigue moviéndose y desacelerando en todas las fases
  ballPos     += deadBallVel * TT_UPDATE_MS;
  deadBallVel *= 0.95f;
  if (fabsf(deadBallVel) < 0.002f) deadBallVel = 0.0f;

  if (deadBallVel > 0.0f && ballPos >= (float)netPos) {
    deadBallVel = -deadBallVel;
    ballPos     = 2.0f * netPos - ballPos;
  } else if (deadBallVel < 0.0f && ballPos <= (float)netPos) {
    deadBallVel = -deadBallVel;
    ballPos     = 2.0f * netPos - ballPos;
  }
  if (ballPos < 0.0f)            { ballPos = 0.0f;                 deadBallVel = 0.0f; }
  if (ballPos >= (float)numLeds) { ballPos = (float)(numLeds - 1); deadBallVel = 0.0f; }

  // ── Fase 3: borrado aleatorio de LEDs ─────────────────────────────
  if (deadEraseCount >= 0) {
    if (deadEraseCount == 0) {
      // Todos borrados → reanuda
      servingPlayer = 1 - loserPlayer;
      serveStart    = millis();
      pos[0]        = TT_SERVE_POS;
      pos[1]        = numLeds - 1 - TT_SERVE_POS;
      vel[0] = vel[1] = 0.0f;
      playerFrozenUntil[0] = playerFrozenUntil[1] = 0;
      ballPos   = (servingPlayer == 0) ? pos[0] + 1.0f : pos[1] - 1.0f;
      shadowPos = ballPos;
      state     = TT_SERVING;
      return;
    }
    if (millis() - deadEraseTimer >= TT_DEAD_ERASE_MS) {
      deadEraseTimer = millis();
      int r = random(0, deadEraseCount);
      playSound(kDeadNotes[random(0, 6)]);
      deadEraseArr[r] = deadEraseArr[--deadEraseCount];
    }
    return;
  }

  // ── Transición fase 2 → fase 3 ────────────────────────────────────
  if (millis() - deadStart >= TT_DEAD_BALL_MS + TT_DEAD_COURT_MS) {
    int halfStart = (loserPlayer == 0) ? 0        : netPos + 1;
    int halfEnd   = (loserPlayer == 0) ? netPos   : numLeds;
    deadEraseCount = 0;
    for (int i = halfStart; i < halfEnd; i++)
      deadEraseArr[deadEraseCount++] = i;
    deadEraseTimer = millis();
  }
}

// ── Update principal ──────────────────────────────────────────────────
void TiraTenis::update() {
  if (introActive) { updateIntro(); return; }
  unsigned long now = millis();
  if (now - lastUpdate < TT_UPDATE_MS) return;
  lastUpdate = now;

  if      (state == TT_SERVING)   updateServe();
  else if (state == TT_HITTING)   updateHitting();
  else if (state == TT_RALLY)     updateRally();
  else if (state == TT_BALL_DEAD) updateDead();

  renderFrame();
}

// ── Input ─────────────────────────────────────────────────────────────
void TiraTenis::onInput(int player, int button) {
  if (introActive) return;
  if (player < 1 || player > 2) return;
  int p = player - 1;
  if (state == TT_SERVING && p == servingPlayer)
    triggerHit(p, true);
  if (state == TT_RALLY) {
    float zoneNear = (p == 0) ? pos[p] + 1 : pos[p] - TT_HIT_LEN;
    float zoneFar  = (p == 0) ? pos[p] + TT_HIT_LEN : pos[p] - 1;
    if (ballPos >= min(zoneNear, zoneFar) && ballPos <= max(zoneNear, zoneFar)) {
      triggerHit(p, false, button == 1);
    } else if (!hitAnimActive) {
      hittingPlayer   = p;
      hitIsLob        = (button == 1);
      hitAnimStep     = 0;
      hitAnimPhase    = 0;
      hitAnimStepTime = millis();
      hitAnimActive   = true;
      playerFrozenUntil[p] = millis() + 2UL * TT_HIT_MS;
    }
  }
}

void TiraTenis::onAnalog(int player, int16_t /*tiltX*/, int16_t tiltY) {
  if (introActive) return;
  if (player < 1 || player > 2) return;
  int p = player - 1;
  if (abs(tiltY) < 800) tilt[p] = 0.0f;
  else                  tilt[p] = constrain(tiltY / 16384.0f, -1.0f, 1.0f);
}

// ── Render ────────────────────────────────────────────────────────────
void TiraTenis::renderFrame() {
  clearLeds();

  // ── Game over: render especial ────────────────────────────────────
  if (state == TT_BALL_DEAD) {
    int lp0    = (int)round(pos[0]);
    int lp1    = (int)round(pos[1]);
    int ballLed = constrain((int)round(ballPos), 0, numLeds - 1);

    if (deadEraseCount >= 0) {
      // Fase 3: LEDs restantes en rojo, resto apagado
      for (int i = 0; i < deadEraseCount; i++)
        setLed(deadEraseArr[i], CRGB::Red);
      if (loserPlayer == 0) {
        setLed(lp1 - 1, CRGB::White); setLed(lp1, CRGB::White);
        if (ballLed > netPos) setLed(ballLed, CRGB::Red);
      } else {
        setLed(lp0, CRGB::White); setLed(lp0 + 1, CRGB::White);
        if (ballLed < netPos) setLed(ballLed, CRGB::Red);
      }
      setLed(netPos, CRGB(40, 40, 40));
    } else if (millis() - deadStart < TT_DEAD_BALL_MS) {
      // Fase 1: jugadores y red normales, pelota roja moviéndose
      setLed(lp0, CRGB::White);     setLed(lp0 + 1, CRGB::White);
      setLed(lp1 - 1, CRGB::White); setLed(lp1, CRGB::White);
      setLed(netPos, CRGB(40, 40, 40));
      setLed(ballLed, CRGB::Red);
    } else {
      // Fase 2: mitad perdedora roja, player perdedor apagado
      if (loserPlayer == 0) {
        for (int i = 0; i < netPos; i++) setLed(i, CRGB::Red);
        setLed(lp0, CRGB::Black); setLed(lp0 + 1, CRGB::Black);
        setLed(lp1 - 1, CRGB::White); setLed(lp1, CRGB::White);
        if (ballLed >= netPos) setLed(ballLed, CRGB::Red);
      } else {
        for (int i = netPos + 1; i < numLeds; i++) setLed(i, CRGB::Red);
        setLed(lp1 - 1, CRGB::Black); setLed(lp1, CRGB::Black);
        setLed(lp0, CRGB::White); setLed(lp0 + 1, CRGB::White);
        if (ballLed <= netPos) setLed(ballLed, CRGB::Red);
      }
      setLed(netPos, CRGB(40, 40, 40));
    }
    showLeds();
    return;
  }

  // ── Render normal ─────────────────────────────────────────────────
  int p1 = (int)round(pos[0]);
  setLed(p1,     CRGB::White);
  setLed(p1 + 1, CRGB::White);

  int p2 = (int)round(pos[1]);
  setLed(p2 - 1, CRGB::White);
  setLed(p2,     CRGB::White);

  // Zona de golpe: barrido azul→blanco (ida) / blanco→apagado (vuelta)
  if (state == TT_HITTING || hitAnimActive) {
    int base  = (int)round(pos[hittingPlayer]);
    int dir   = (hittingPlayer == 0) ? 1 : -1;
    CRGB sweepColor = hitIsLob ? CRGB::Green : CRGB::Blue;
    for (int i = 1; i <= TT_HIT_LEN; i++) {
      if (i <= hitAnimStep)       setLed(base + dir * i, CRGB::White);
      else if (hitAnimPhase == 0) setLed(base + dir * i, sweepColor);
    }
  }

  setLed(netPos, CRGB(40, 40, 40));

  // Marca de pique: amarillo 10%, dura 1 segundo
  if (bounceMarkTime > 0 && millis() - bounceMarkTime < 1000)
    setLed((int)round(bounceMarkPos), CRGB(26, 26, 0));

  if (state == TT_RALLY)
    setLed((int)round(shadowPos), CRGB(13, 13, 0));

  // Pelota
  if (state == TT_RALLY && isLob && (millis() - rallyStartTime < pendingTotalMs / 2.0f)) {
    int b = (int)round(ballPos);
    setLed(b - 1, CRGB::OrangeRed);
    setLed(b,     CRGB::OrangeRed);
    setLed(b + 1, CRGB::OrangeRed);
  } else {
    setLed((int)round(ballPos), CRGB::Yellow);
  }

  showLeds();
}
