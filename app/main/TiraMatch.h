#pragma once
#include "GameBase.h"

#define MAX_SHOTS        10   // disparos simultáneos en vuelo (ambos jugadores)
#define SHOT_LEN         4    // largo del disparo en LEDs
#define SHOT_SPEED       10   // ms por paso del disparo (menos = más rápido)
#define SPAWN_INTERVAL     280  // ms inicial entre spawns (velocidad de arranque)
#define SPAWN_INTERVAL_MIN  30  // ms mínimo entre spawns (velocidad máxima)
#define SPAWN_ACCEL          0.5f  // ms que se reduce el intervalo por cada spawn (valor inicial)
#define SPAWN_ACCEL_MATCH    0.1f // cuánto sube/baja spawnAccel por match/error
#define MAX_SPAWN_ACCEL      0.8f  // techo de spawnAccel
#define SEGMENT_LEN      8    // LEDs por bloque de color antes de cambiar
#define EROSION_INTERVAL 4    // spawns del mismo jugador antes de borrar su LED más viejo
#define SHIFT_STEPS      10   // LEDs desplazados hacia el oponente en un match
#define SHIFT_STEP_MS    20   // ms por paso del desplazamiento (10 pasos = 200ms)

#define POT_PIN         34    // GPIO ADC para potenciómetro de dificultad
#define POT_READ_MS    200    // intervalo de lectura del pot (ms)

// Sonidos del juego (formato RTTTL) — se reproducen en master Y se envían a los joysticks
#define SND_SHOOT     "Shoot:d=16,o=5,b=160:b5,g5,d5"
#define SND_COLOR     "Color:d=32,o=5,b=160:c5"
#define SND_MISS      "Miss:d=16,o=4,b=120:e4,b3"
#define SND_EXPLOSION "Expl:d=16,o=3,b=100:c4,a3,g3,f3"

// Explosión de partículas en match
#define MAX_PARTICLES   16     // partículas simultáneas máximas
#define PARTICLE_DECEL  0.85f  // factor de desaceleración por frame (menor = frena más rápido)
#define PARTICLE_MS     16     // ms entre actualizaciones de partículas (~60fps)

enum GoPhase { GO_NONE, GO_BLINK, GO_SWEEP, GO_VICTORY, GO_VICTORY_WAIT, GO_VICTORY_OFF, GO_DONE };

struct Particle {
  float   pos;     // posición actual (float para movimiento suave)
  float   vel;     // velocidad actual (LEDs por frame)
  float   maxVel;  // velocidad inicial (para calcular brillo)
  int     dir;     // dirección: -1 hacia P1, +1 hacia P2
  bool    active;
};

struct Shot {
  int  pos;
  int  dir;
  bool active;
  CRGB color;
  int  colorIdx;
};

class TiraMatch : public GameBase {
public:
  TiraMatch(CRGB* leds, int numLeds, U8G2* display);
  ~TiraMatch();

  void begin()  override;
  void update() override;
  void onInput(int player, int button) override;

private:
  Shot          shots[MAX_SHOTS];
  unsigned long lastMove;

  int p1ColorIdx;
  int p2ColorIdx;

  CRGB*         spawnBuffer;
  float         spawnIntervalMs; // intervalo actual (decrece con SPAWN_ACCEL)
  float         difficulty;      // 0.0=fácil, 1.0=difícil
  int           effectiveSegLen; // SEGMENT_LEN ajustado por dificultad (= estado actual del buffer)
  float         spawnAccel;      // aceleración actual del spawn (sube con matches, baja con errores)
  float         spawnMult;       // multiplicador de velocidad de spawn
  unsigned long lastPotRead;
  int           spawnTurn;       // 0=izquierda, 1=derecha
  int           leftLedInSeg;
  int           rightLedInSeg;
  int           leftSpawnCount;  // total de LEDs spawneados en lado izquierdo
  int           rightSpawnCount;
  int           leftColorIdx;
  int           rightColorIdx;
  unsigned long lastSpawn;

  Particle      particles[MAX_PARTICLES];
  unsigned long lastParticleUpdate;

  int           spawnCenter;      // posición del spawner (se mueve con cada match)
  int           shiftRemaining;
  int           shiftDir;
  unsigned long lastShift;

  bool          introActive;
  unsigned long introStart;
  unsigned long lastIntroSound;

  bool          gameOver;
  int           loser;
  GoPhase       goPhase;
  int           goBlinkCount;
  bool          goBlinkOn;
  unsigned long goTimer;
  int           goSweepStep;

  static const CRGB COLORS[6];
  int           activeColors;
  unsigned long lastColorAdd;

  void fire(int player);
  void moveShots();
  void checkCollisions();
  void explodeAndDestroy(int hitIdx, int minBound, int maxBound, int explosionDir);
  void spawnExplosion(int segStart, int segLen, int dir);
  void updateParticles();
  void renderParticles();
  int  findFreeParticle();
  void spawnNextLed();
  void addPenalty(int player, CRGB color);
  void startShift(int dir, int steps);
  void stepShift();
  void erodeLeft();
  void erodeRight();
  bool checkGameOver();
  void renderFrame();
  void renderSpawner();
  void updateIntro();
  void updateGameOver();
  void renderBlinkFrame();
  void renderGameOver();
  void updateDisplay();
  int  findFreeSlot();
  int  pickOtherColor(int current);
  int  nextColorInCycle(int current);
  void readDifficulty();
  void rescaleBuffer(int oldLen, int newLen);
};
