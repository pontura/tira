#pragma once
#include "GameBase.h"

// 1 arquero + 2 defensores + 3 delanteros por equipo
#define MG_TEAM_SIZE     6
#define MG_MAX_PLAYERS  (MG_TEAM_SIZE * 2)

#define MG_GK_OFFSET     5    // LEDs desde cada punta de la tira hasta el arquero
#define MG_BACK_LEDS     5    // cuánto retrocede en el amague
#define MG_KICK_LEDS    10    // cuánto avanza en la patada (neto: termina MG_BACK_LEDS pasado el home)

#define MG_BACKSWING_MS 150   // amague: rápido → lento
#define MG_KICK_MS      200   // patada: lento → rápido → lento
#define MG_RETURN_MS    150   // vuelta a home: lento → rápido

#define MG_UPDATE_MS      16  // tope de framerate (~60fps)
#define MG_IDLE_BRIGHTNESS 13  // ~5% de 255, color del player en reposo/amague/vuelta

// ── Pelota ────────────────────────────────────────────────────────────
#define MG_BALL_LAUNCH_SPEED  2.0f    // LEDs/tick al lanzarla al arranque de la jugada (100% del rango)
#define MG_BALL_GOAL_PCT      0.375f  // % de MG_BALL_LAUNCH_SPEED al salir tras un gol: promedio de 25%-50%, sin azar
#define MG_BALL_KICK_SPEED    2.0f    // LEDs/tick máx.: pelota justo en el home del player que patea
#define MG_BALL_KICK_SPEED_MIN 0.6f   // LEDs/tick mín.: pelota en el límite del recorrido (±MG_BACK_LEDS)
#define MG_BALL_DECEL         0.9767f // fricción: fracción de velocidad que conserva cada tick
#define MG_BALL_MIN_VEL       0.02f   // por debajo de esto, se la considera detenida

// Sonido de patada en el master: 5 niveles de "golpe", de más fuerte a más
// débil, según qué tan cerca del home del player haya sido el contacto.
#define SND_MG_KICK1 "Kick1:d=8,o=3,b=200:c,g2"
#define SND_MG_KICK2 "Kick2:d=8,o=4,b=200:c"
#define SND_MG_KICK3 "Kick3:d=16,o=4,b=200:e"
#define SND_MG_KICK4 "Kick4:d=16,o=5,b=200:g"
#define SND_MG_KICK5 "Kick5:d=32,o=6,b=200:c"

// ── Gol ───────────────────────────────────────────────────────────────
#define MG_GOAL_GROW_MS          1200 // tiempo en que el equipo que anotó cubre toda la tira
#define MG_GOAL_HOLD_MS          3000 // duración total del festejo (mientras suena la música)
#define MG_GOAL_RESTART_DELAY_MS 1000 // pausa entre el reinicio y la salida de la pelota
#define SND_MG_GOAL "Goal:d=8,o=5,b=120:c,e,g,c6,e6,g6,c6,g6,e6,c6,g,c"

enum MG_Role  { MG_ROLE_GK, MG_ROLE_DEF, MG_ROLE_FWD };
enum MG_State { MG_STATE_IDLE, MG_STATE_BACKSWING, MG_STATE_KICK, MG_STATE_RETURN };
enum MG_Phase { MG_PHASE_PLAY, MG_PHASE_GOAL, MG_PHASE_RESTART_WAIT };

struct MG_Ball {
  float pos;
  float vel;  // LEDs/tick; + hacia la derecha, - hacia la izquierda
};

struct MG_Player {
  int   team;      // 0 = amarillo, 1 = cian
  int   role;      // MG_Role: define qué botón lo mueve
  int   dir;       // +1 (amarillo, ataca hacia la derecha) o -1 (cian, ataca hacia la izquierda)
  int   homeLed;   // posición de reposo

  MG_State      state;
  float         pos;          // posición actual animada (float, LEDs)
  unsigned long phaseStart;
  float         phaseFrom;
  float         phaseTo;
  unsigned long phaseDurMs;
  bool          hasHitBall;   // ya le pegó a la pelota en esta patada (máx. 1 golpe por animación)
};

class Metegol : public GameBase {
public:
  Metegol(CRGB* leds, int numLeds, U8G2* display);
  void begin()  override;
  void update() override;
  void onInput(int player, int button) override;

private:
  MG_Player     players[MG_MAX_PLAYERS];
  MG_Ball       ball;
  unsigned long lastUpdate;

  MG_Phase      phase;
  unsigned long goalPhaseStart;  // millis() al entrar en MG_PHASE_GOAL o MG_PHASE_RESTART_WAIT
  int           scoringTeam;     // equipo que anotó el gol vigente
  bool          goalWasLeft;     // true si la pelota entró por el extremo izquierdo
  float         goalMaxRadius;   // radio necesario para que el equipo que anotó cubra toda la tira

  void triggerKick(MG_Player& p);
  void updatePlayer(MG_Player& p);
  bool updateBall();  // true si la pelota llegó a un extremo (gol) en este frame
  void launchFromCenter(int dir);        // saque normal: dirección dada, velocidad al azar (25%-50%)
  void relaunchTowardGoal(bool leftGoal); // saque post-gol: hacia el arco que lo recibió, velocidad fija
  void triggerGoal(bool leftGoal);
  void updateGoalCelebration();
  void resetForKickoff();
  void renderFrame();
};
