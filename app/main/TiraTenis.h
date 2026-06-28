#pragma once
#include "GameBase.h"

// ── Posición inicial (LEDs desde el extremo de cada jugador) ──────────
#define TT_P1_START          20     // P1 empieza a 20 LEDs del extremo izquierdo
#define TT_P2_START          20     // P2 empieza a 20 LEDs del extremo derecho

// ── Parámetros de movimiento ──────────────────────────────────────────
#define TT_ACCEL             0.4f   // ganancia de velocidad por frame con botón presionado (LEDs/frame²)
#define TT_DECEL             0.82f  // fricción por frame al soltar (0=para instantáneo, 1=no para)
#define TT_MAX_SPEED         0.8f   // velocidad máxima del jugador (LEDs/frame)

// ── Parámetros del saque ──────────────────────────────────────────────
#define TT_SERVE_PERIOD_MS   1600   // duración de un ciclo completo de la animación del saque (ms)
#define TT_SERVE_TRAVEL      20     // LEDs totales que recorre la pelota durante el saque

// ── Parámetros del golpe ──────────────────────────────────────────────
#define TT_HIT_LEN           10     // LEDs azules del hit (zona de golpe)
#define TT_HIT_MS            500    // duración del efecto visual del golpe (ms)

// ── Parámetros de la pelota en rally ─────────────────────────────────
#define TT_BALL_MAX_SPEED    3.2f   // velocidad al 100% de fuerza (LEDs/frame)
#define TT_BALL_MIN_SPEED    0.4f   // velocidad al 10% de fuerza (LEDs/frame)
#define TT_BALL_DECEL        0.99f // fricción de la pelota por frame en rally (1.0=sin fricción)
#define TT_HIT_RANGE         20     // distancia máxima al jugador para que el hit sea válido (LEDs)

// ── Timing ────────────────────────────────────────────────────────────
#define TT_UPDATE_MS         16     // ms por frame (~60fps)

#define TT_TRAIL_MAX_LEN     5      // LEDs máximos de trail (a velocidad máxima)
#define TT_BALL_STUCK_MS     200    // ms en el mismo LED para considerar la pelota muerta
#define TT_DEAD_MS          1000    // ms que dura el freeze de pelota muerta

enum TennisState { TT_SERVING, TT_HITTING, TT_RALLY, TT_BALL_DEAD };

class TiraTenis : public GameBase {
public:
  TiraTenis(CRGB* leds, int numLeds, U8G2* display);
  void begin()                            override;
  void update()                           override;
  void onInput(int player, int button)    override;
  void onButtonUp(int player, int button) override;

private:
  TennisState   state;
  int           servingPlayer;  // 0=P1, 1=P2
  float         pos[2];         // posición de cada jugador (float para suavidad)
  float         vel[2];         // velocidad del jugador
  bool          held[2][2];     // held[player][button]
  float         ballPos;        // posición actual de la pelota
  float         ballVel;        // velocidad de la pelota en rally (LEDs/frame, signed)
  unsigned long serveStart;     // momento en que arrancó el saque actual
  unsigned long hitStart;       // momento en que arrancó el golpe
  int           hittingPlayer;  // quién está golpeando (0 o 1)
  bool          hitSuccess;     // si la pelota estaba en la zona al golpear
  float         hitStrength;    // 0.1..1.0 según posición de la pelota en zona
  bool          hitFromServe;   // true si el hit fue desde saque (para saber a qué volver)
  int           lastBallLed;    // último LED entero donde estuvo la pelota
  unsigned long sameledStart;   // millis() cuando la pelota llegó al LED actual
  unsigned long deadStart;      // millis() cuando arrancó el estado de pelota muerta
  int           netPos;            // LED central (red)
  unsigned long lastUpdate;

  void  updateServe();
  void  updateHitting();
  void  updateRally();
  void  updateDead();
  void  triggerHit(int p, bool fromServe);
  void  renderFrame();
  float serveBallFraction(unsigned long elapsed);
};
