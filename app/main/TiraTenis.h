#pragma once
#include "GameBase.h"

// ── Posición inicial ───────────────────────────────────────────────────
#define TT_SERVE_POS         6    // posición de saque (LEDs desde el extremo)

// ── Movimiento del jugador ─────────────────────────────────────────────
#define TT_ACCEL             0.4f
#define TT_DECEL             0.82f
#define TT_MAX_SPEED         0.8f

// ── Saque ──────────────────────────────────────────────────────────────
#define TT_SERVE_PERIOD_MS   1600
#define TT_SERVE_TRAVEL      20

// ── Hit (zona azul) ───────────────────────────────────────────────────
#define TT_HIT_LEN           13
#define TT_HIT_MS            200

// ── Trayectoria de la pelota ───────────────────────────────────────────
#define TT_BALL_LEDS_MAX     200    // LEDs totales al 100% de potencia (drive)
#define TT_BALL_LEDS_MIN     25     // LEDs totales al 10% de potencia (drive)
#define TT_BALL_TIME_MAX_MS  2200   // ms al 100% de potencia (drive)
#define TT_BALL_TIME_MIN_MS  1800   // ms al 10% de potencia (drive)
#define TT_SERVE_LEDS_MAX    200    // LEDs totales al 100% de potencia (saque)
#define TT_SERVE_LEDS_MIN    80    // LEDs totales al 10% de potencia (saque)
#define TT_LOB_LEDS_MAX      150    // LEDs totales al 100% de potencia (lob)
#define TT_LOB_LEDS_MIN      30    // LEDs totales al 10% de potencia (lob)
#define TT_LOB_TIME_MAX_MS   4000   // ms al 100% de potencia (lob)
#define TT_LOB_TIME_MIN_MS   2200   // ms al 10% de potencia (lob)

// ── Pelota muerta ─────────────────────────────────────────────────────
#define TT_DEAD_BALL_MS       250    // fase 1: pelota roja moviéndose
#define TT_DEAD_COURT_MS      500    // fase 2: cancha del perdedor roja
#define TT_DEAD_ERASE_MS      15     // fase 3: ms por LED borrado aleatorio
#define TT_POST_HIT_FREEZE_MS 1000
#define TT_WIN_SCORE          5
#define TT_GO_STEP_MS         30
#define TT_GO_WAIT_MS         3000

#define SND_HIT      "Hit:d=32,o=6,b=220:c6,c7"
#define SND_LOB      "Lob:d=16,o=4,b=180:c4,g4"
#define SND_GAMEOVER "game over:o=5,d=32,b=400:c#,d,4e,4c#,4d,4b4,4c#,4a4,4b4,4p"

// ── Timing ────────────────────────────────────────────────────────────
#define TT_UPDATE_MS         16

enum TennisState { TT_SERVING, TT_HITTING, TT_RALLY, TT_BALL_DEAD, TT_GAME_OVER };

class TiraTenis : public GameBase {
public:
  TiraTenis(CRGB* leds, int numLeds, U8G2* display);
  void begin()                            override;
  void update()                           override;
  void onInput(int player, int button)    override;
  void onAnalog(int player, int16_t tiltX, int16_t tiltY) override;

private:
  TennisState   state;
  int           servingPlayer;
  unsigned long serveStart;
  float         pos[2];
  float         vel[2];
  float         tilt[2];
  int           netPos;
  unsigned long lastUpdate;

  // ── Pelota ────────────────────────────────────────────────────────
  float         ballPos;
  float         shadowPos;
  int           ballDir;        // +1 o -1

  // Trayectoria pendiente (se guarda durante TT_HITTING)
  float         pendingBallStart;
  float         pendingTotalLeds;
  float         pendingTotalMs;
  int           lastHitter;
  bool          isLob;
  unsigned long rallyStartTime;
  unsigned long playerFrozenUntil[2]; // jugador congelado hasta este timestamp

  // Segmento actual de la trayectoria
  int           ballSeg;          // 0 o 1
  float         segStartPos;      // posición del inicio del segmento
  float         segLen;           // LEDs del segmento (sin signo)
  float         segMs;            // duración del segmento
  unsigned long segStartTime;
  float         shadowSegStart;   // posición inicial de la sombra en este segmento

  // Marca de pique
  float         bounceMarkPos;
  unsigned long bounceMarkTime;

  // Pelota muerta
  unsigned long deadStart;
  int           loserPlayer;
  float         deadBallVel;
  int           deadEraseArr[144]; // índices de LEDs restantes en fase 3
  int           deadEraseCount;    // -1=no iniciada, ≥0=LEDs restantes
  unsigned long deadEraseTimer;

  // Hit
  bool          hitSuccess;
  float         hitStrength;
  bool          hitFromServe;
  bool          hitIsLob;
  int           hittingPlayer;
  unsigned long hitStart;
  int           hitAnimStep;       // LEDs blancos activos (0..TT_HIT_LEN)
  int           hitAnimPhase;      // 0=avanzando, 1=retrocediendo
  unsigned long hitAnimStepTime;
  bool          hitAnimActive;     // animación de miss corriendo durante TT_RALLY

  int           score[2];
  int           winner;
  int           goPhase;
  unsigned long goTimer;
  int           goFillStep;
  int           goEraseArr[144];
  int           goEraseCount;

  bool          introActive;
  int           introPhase;        // 0=approach, 1=walkin
  unsigned long introStart;
  unsigned long introPhaseStart;

  void  updateIntro();
  void  updateGameOver();
  void  renderScoreLeds();
  void  updateServe();
  void  updateHitting();
  void  updateRally();
  void  updateDead();
  void  startSegment(int seg);
  void  triggerHit(int p, bool fromServe, bool lob = false);
  void  renderFrame();
  float serveBallFraction(unsigned long elapsed);
};
