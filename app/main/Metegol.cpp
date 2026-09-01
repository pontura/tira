#include "Metegol.h"
#include <math.h>

Metegol::Metegol(CRGB* leds, int numLeds, U8G2* display)
  : GameBase(leds, numLeds, display) {}

// Arqueros a MG_GK_OFFSET LEDs de cada punta. Los 10 jugadores de campo se
// reparten parejos entre los dos arqueros de forma que, desde cada extremo,
// se vea: arquero, atacante rival, defensor propio, atacante rival, defensor
// propio, atacante rival — y luego, ya del lado del arco rival, lo mismo mirado
// al revés: atacante propio, defensor rival, atacante propio, defensor rival,
// atacante propio. Así cada defensor marca de cerca a los delanteros rivales
// que atacan su propio arco.
void Metegol::begin() {
  int gkYellow = MG_GK_OFFSET;
  int gkCyan   = numLeds - 1 - MG_GK_OFFSET;
  float step   = (float)(gkCyan - gkYellow) / (float)(MG_TEAM_SIZE * 2 - 1);  // 11 tramos

  // team: 0 = amarillo, 1 = cian. Posiciones 1..10, de izquierda (arco
  // amarillo) a derecha (arco cian).
  static const int fieldTeam[10] = { 1, 0, 1, 0, 1,  0, 1, 0, 1, 0 };
  static const int fieldRole[10] = {
    MG_ROLE_FWD, MG_ROLE_DEF, MG_ROLE_FWD, MG_ROLE_DEF, MG_ROLE_FWD,
    MG_ROLE_FWD, MG_ROLE_DEF, MG_ROLE_FWD, MG_ROLE_DEF, MG_ROLE_FWD
  };

  int idx = 0;
  auto setup = [&](int team, int role, int dir, int home) {
    MG_Player& p = players[idx++];
    p.team       = team;
    p.role       = role;
    p.dir        = dir;
    p.homeLed    = home;
    p.state      = MG_STATE_IDLE;
    p.pos        = (float)home;
    p.phaseStart = 0;
    p.phaseFrom  = (float)home;
    p.phaseTo    = (float)home;
    p.phaseDurMs = 0;
    p.hasHitBall = false;
  };

  setup(0, MG_ROLE_GK, +1, gkYellow);
  setup(1, MG_ROLE_GK, -1, gkCyan);

  for (int i = 1; i <= MG_TEAM_SIZE * 2 - 2; i++) {
    int led  = gkYellow + (int)roundf(step * (float)i);
    int team = fieldTeam[i - 1];
    int dir  = (team == 0) ? +1 : -1;
    setup(team, fieldRole[i - 1], dir, led);
  }

  // La pelota arranca en el centro y se lanza una sola vez, alternando de lado
  // en cada partida (cada begin()): primera vez a la derecha, la siguiente a
  // la izquierda, y así sucesivamente mientras el dispositivo siga encendido.
  static int nextKickoffDir = +1;
  launchFromCenter(nextKickoffDir);
  nextKickoffDir = -nextKickoffDir;

  phase      = MG_PHASE_PLAY;
  lastUpdate = millis();
  clearLeds();
  renderFrame();
}

// Cualquiera de los 2 botones dispara la patada del grupo que le corresponde:
// botón 0 = arquero + defensores, botón 1 = los 3 delanteros. Un player que ya
// está pateando ignora el nuevo input hasta volver a quedar en reposo. Fuera
// de MG_PHASE_PLAY (festejo de gol / pausa antes del saque) los inputs se
// ignoran: el juego queda congelado.
void Metegol::onInput(int player, int button) {
  if (phase != MG_PHASE_PLAY) return;
  if (player < 1 || player > 2) return;
  int team = player - 1;

  for (int i = 0; i < MG_MAX_PLAYERS; i++) {
    MG_Player& p = players[i];
    if (p.team != team) continue;
    bool matchesButton = (button == 0) ? (p.role == MG_ROLE_GK || p.role == MG_ROLE_DEF)
                                        : (p.role == MG_ROLE_FWD);
    if (matchesButton) triggerKick(p);
  }
}

void Metegol::triggerKick(MG_Player& p) {
  if (p.state != MG_STATE_IDLE) return;
  p.state      = MG_STATE_BACKSWING;
  p.phaseStart = millis();
  p.phaseFrom  = (float)p.homeLed;
  p.phaseTo    = (float)p.homeLed - (float)(p.dir * MG_BACK_LEDS);
  p.phaseDurMs = MG_BACKSWING_MS;
  p.hasHitBall = false;
}

// Amague (rápido→lento) → patada (lento→rápido→lento, 100% opaco) → vuelta a
// home (lento→rápido). El color de "idle" (tenue) se dibuja en cualquier
// estado menos MG_STATE_KICK.
void Metegol::updatePlayer(MG_Player& p) {
  if (p.state == MG_STATE_IDLE) { p.pos = (float)p.homeLed; return; }

  unsigned long now = millis();
  float t = (float)(now - p.phaseStart) / (float)p.phaseDurMs;
  t = constrain(t, 0.0f, 1.0f);

  float eased;
  if      (p.state == MG_STATE_BACKSWING) eased = 1.0f - (1.0f - t) * (1.0f - t);   // ease-out
  else if (p.state == MG_STATE_KICK)      eased = t * t * (3.0f - 2.0f * t);        // ease-in-out
  else                                    eased = t * t;                             // ease-in (RETURN)

  p.pos = p.phaseFrom + (p.phaseTo - p.phaseFrom) * eased;
  if (t < 1.0f) return;

  if (p.state == MG_STATE_BACKSWING) {
    p.state      = MG_STATE_KICK;
    p.phaseStart = now;
    p.phaseFrom  = p.phaseTo;
    p.phaseTo    = (float)p.homeLed + (float)(p.dir * MG_BACK_LEDS);
    p.phaseDurMs = MG_KICK_MS;
  } else if (p.state == MG_STATE_KICK) {
    p.state      = MG_STATE_RETURN;
    p.phaseStart = now;
    p.phaseFrom  = p.phaseTo;
    p.phaseTo    = (float)p.homeLed;
    p.phaseDurMs = MG_RETURN_MS;
  } else {  // RETURN completo
    p.state = MG_STATE_IDLE;
    p.pos   = (float)p.homeLed;
  }
}

// Un player solo afecta a la pelota si en ese instante está en MG_STATE_KICK,
// todavía no le pegó en esta misma animación (hasHitBall) y su LED cae dentro
// del tramo que la pelota recorrió en este frame; le imprime velocidad hacia
// el arco rival (su propio "dir", el lado contrario al de su arquero). El
// flag hasHitBall se resetea al iniciar cada patada (triggerKick) y garantiza
// que un player nunca golpee la pelota más de una vez por animación, aunque
// ambos se solapen varios frames seguidos. La colisión se chequea contra todo
// el tramo recorrido
// (no solo la posición final) porque a velocidad alta la pelota puede avanzar
// más de un LED por frame y "saltar" sobre el LED del player si solo se
// comparara la posición exacta de llegada. La fuerza del golpe depende de qué
// tan cerca esté la pelota del home del player en el momento de la colisión:
// justo en el home sale a MG_BALL_KICK_SPEED, y va bajando linealmente hasta
// MG_BALL_KICK_SPEED_MIN al llegar a los extremos del recorrido de patada
// (±MG_BACK_LEDS). Si la pelota ya viaja en la misma dirección que patea el
// player, esa fuerza se suma a la velocidad que ya traía (nunca se la resta);
// si viene detenida o en dirección contraria, la patada directamente le fija
// esa velocidad. Fuera de eso, la pelota sigue solo por inercia, perdiendo
// velocidad por fricción. Si se detiene sola (sin que nadie la haya tocado),
// vuelve a salir del centro en una dirección al azar. Cada patada suena en el
// master con uno de 5 golpes posibles, de más fuerte a más débil según esa
// misma distancia. Devuelve true si la pelota llegó a
// alguno de los dos extremos de la tira en este frame (gol) — en ese caso no
// se aplica el auto-relanzamiento por fricción, porque el gol maneja su
// propio reinicio.
bool Metegol::updateBall() {
  static const char* kickSounds[5] = {
    SND_MG_KICK1, SND_MG_KICK2, SND_MG_KICK3, SND_MG_KICK4, SND_MG_KICK5
  };

  bool  wasStopped = (ball.vel == 0.0f);
  float prevPos    = ball.pos;

  ball.pos += ball.vel;
  ball.vel *= MG_BALL_DECEL;
  if (fabsf(ball.vel) < MG_BALL_MIN_VEL) ball.vel = 0.0f;
  ball.pos = constrain(ball.pos, 0.0f, (float)(numLeds - 1));

  int loLed = (int)roundf(fminf(prevPos, ball.pos));
  int hiLed = (int)roundf(fmaxf(prevPos, ball.pos));

  for (int i = 0; i < MG_MAX_PLAYERS; i++) {
    MG_Player& p = players[i];
    if (p.state != MG_STATE_KICK || p.hasHitBall) continue;
    int pLed = (int)roundf(p.pos);
    if (pLed < loLed || pLed > hiLed) continue;

    float dist    = fabsf((float)pLed - (float)p.homeLed);
    float t       = constrain(dist / (float)MG_BACK_LEDS, 0.0f, 1.0f);
    float speed   = MG_BALL_KICK_SPEED - (MG_BALL_KICK_SPEED - MG_BALL_KICK_SPEED_MIN) * t;
    bool  sameDir = (p.dir > 0 && ball.vel > 0.0f) || (p.dir < 0 && ball.vel < 0.0f);
    if (sameDir) ball.vel += (float)p.dir * speed;
    else         ball.vel  = (float)p.dir * speed;
    ball.pos      = (float)pLed;  // el contacto ocurre justo en el LED del player
    p.hasHitBall  = true;         // un solo golpe por animación de patada

    int tier = constrain((int)(t * 5.0f), 0, 4);
    playSound(kickSounds[tier]);
  }

  if (ball.pos <= 0.0f || ball.pos >= (float)(numLeds - 1)) return true;

  if (!wasStopped && ball.vel == 0.0f) {
    int dir = (random(0, 2) == 0) ? -1 : +1;
    launchFromCenter(dir);
  }
  return false;
}

// Saque normal desde el centro: dirección dada (usada para el kickoff y, con
// una dirección al azar, para el reinicio por fricción), a una velocidad al
// azar entre 1/4 y 1/2 de MG_BALL_LAUNCH_SPEED.
void Metegol::launchFromCenter(int dir) {
  float pct   = (float)random(25, 51) / 100.0f;  // 0.25 .. 0.50
  float speed = MG_BALL_LAUNCH_SPEED * pct;
  ball.pos = numLeds / 2.0f;
  ball.vel = (float)dir * speed;
}

// Saque post-gol: siempre hacia el mismo extremo que acaba de recibir el gol
// (nunca al azar), a una velocidad fija (sin azar) igual al promedio del rango
// usado en el saque normal.
void Metegol::relaunchTowardGoal(bool leftGoal) {
  int dir = leftGoal ? -1 : +1;
  ball.pos = numLeds / 2.0f;
  ball.vel = (float)dir * MG_BALL_LAUNCH_SPEED * MG_BALL_GOAL_PCT;
}

// El equipo que anotó es el dueño del arco opuesto al extremo donde entró la
// pelota (dir +1 ataca el extremo derecho, dir -1 el izquierdo). El radio
// necesario para que ese equipo cubra toda la tira se calcula una sola vez acá
// (no en cada frame de la animación): es la mayor distancia, entre todos los
// LEDs, al jugador de ese equipo más cercano.
void Metegol::triggerGoal(bool leftGoal) {
  scoringTeam = leftGoal ? 1 : 0;
  goalWasLeft = leftGoal;

  float maxNeeded = 0.0f;
  for (int led = 0; led < numLeds; led++) {
    float minDist = 1e9f;
    for (int i = 0; i < MG_MAX_PLAYERS; i++) {
      if (players[i].team != scoringTeam) continue;
      float d = fabsf((float)led - (float)players[i].homeLed);
      if (d < minDist) minDist = d;
    }
    if (minDist > maxNeeded) maxNeeded = minDist;
  }
  goalMaxRadius = maxNeeded;

  phase          = MG_PHASE_GOAL;
  goalPhaseStart = millis();
  playSound(SND_MG_GOAL);
}

// Festejo de gol: el equipo que anotó se agranda desde el home de cada uno de
// sus jugadores hasta cubrir toda la tira (ease-out), mientras los LEDs del
// equipo que lo recibió (los que todavía no quedaron tapados por ese avance)
// se apagan en fade. Al cumplirse MG_GOAL_HOLD_MS (duración de la música) se
// reinician las posiciones y se pasa a la pausa previa al próximo saque.
void Metegol::updateGoalCelebration() {
  unsigned long now     = millis();
  unsigned long elapsed = now - goalPhaseStart;

  float growT  = constrain((float)elapsed / (float)MG_GOAL_GROW_MS, 0.0f, 1.0f);
  float eased  = 1.0f - (1.0f - growT) * (1.0f - growT);  // ease-out
  float radius = eased * goalMaxRadius;  // llega exactamente a cubrir todo al final de MG_GOAL_GROW_MS

  const CRGB teamColor[2] = { CRGB::Red, CRGB::Blue };
  CRGB scoreColor = teamColor[scoringTeam];

  fill_solid(leds, numLeds, CRGB(0, 7, 0));

  for (int led = 0; led < numLeds; led++) {
    float minDist = 1e9f;
    for (int i = 0; i < MG_MAX_PLAYERS; i++) {
      if (players[i].team != scoringTeam) continue;
      float d = fabsf((float)led - (float)players[i].homeLed);
      if (d < minDist) minDist = d;
    }
    if (minDist <= radius) leds[led] = scoreColor;
  }

  for (int i = 0; i < MG_MAX_PLAYERS; i++) {
    MG_Player& p = players[i];
    if (p.team == scoringTeam) continue;
    int ledIdx = constrain((int)roundf(p.pos), 0, numLeds - 1);

    bool covered = false;
    for (int j = 0; j < MG_MAX_PLAYERS; j++) {
      if (players[j].team != scoringTeam) continue;
      if (fabsf((float)ledIdx - (float)players[j].homeLed) <= radius) { covered = true; break; }
    }
    if (!covered) {
      CRGB col = teamColor[p.team];
      col.nscale8((uint8_t)(255.0f * (1.0f - growT)));
      leds[ledIdx] = col;
    }
  }

  showLeds();

  if (elapsed >= MG_GOAL_HOLD_MS) {
    resetForKickoff();
    phase          = MG_PHASE_RESTART_WAIT;
    goalPhaseStart = millis();
  }
}

// Vuelve a dejar a todos los jugadores en reposo sobre su home y la pelota
// quieta en el centro, sin lanzarla todavía (eso pasa MG_GOAL_RESTART_DELAY_MS
// más tarde, ya en MG_PHASE_RESTART_WAIT).
void Metegol::resetForKickoff() {
  for (int i = 0; i < MG_MAX_PLAYERS; i++) {
    MG_Player& p = players[i];
    p.state      = MG_STATE_IDLE;
    p.pos        = (float)p.homeLed;
    p.phaseStart = 0;
    p.phaseFrom  = (float)p.homeLed;
    p.phaseTo    = (float)p.homeLed;
    p.phaseDurMs = 0;
    p.hasHitBall = false;
  }
  ball.pos = numLeds / 2.0f;
  ball.vel = 0.0f;
}

void Metegol::update() {
  unsigned long now = millis();
  if (now - lastUpdate < MG_UPDATE_MS) return;
  lastUpdate = now;

  if (phase == MG_PHASE_GOAL) {
    updateGoalCelebration();
    return;
  }

  if (phase == MG_PHASE_RESTART_WAIT) {
    renderFrame();
    if (now - goalPhaseStart >= MG_GOAL_RESTART_DELAY_MS) {
      relaunchTowardGoal(goalWasLeft);
      phase = MG_PHASE_PLAY;
    }
    return;
  }

  for (int i = 0; i < MG_MAX_PLAYERS; i++) updatePlayer(players[i]);
  if (updateBall()) {
    triggerGoal(ball.pos <= 0.0f);
    return;
  }
  renderFrame();
}

void Metegol::renderFrame() {
  fill_solid(leds, numLeds, CRGB(0, 0, 0));  // verde casi negro (visible pero muy tenue)

  const CRGB teamColor[2] = { CRGB::Red, CRGB::Blue };

  for (int i = 0; i < MG_MAX_PLAYERS; i++) {
    MG_Player& p = players[i];
    CRGB col = teamColor[p.team];
    if (p.state != MG_STATE_KICK) col.nscale8(MG_IDLE_BRIGHTNESS);
    setLed(constrain((int)roundf(p.pos), 0, numLeds - 1), col);
  }

  setLed(constrain((int)roundf(ball.pos), 0, numLeds - 1), CRGB::White);

  showLeds();
}
