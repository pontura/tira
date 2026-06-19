#pragma once
#include "GameBase.h"

#define MAX_SHOTS        10   // disparos simultáneos en vuelo (ambos jugadores)
#define SHOT_LEN         4    // largo del disparo en LEDs
#define SHOT_SPEED       10   // ms por paso del disparo (menos = más rápido)
#define SPAWN_INTERVAL     250  // ms inicial entre spawns (velocidad de arranque)
#define SPAWN_INTERVAL_MIN  80  // ms mínimo entre spawns (velocidad máxima)
#define SPAWN_ACCEL          0.5// ms que se reduce el intervalo por cada spawn
#define SEGMENT_LEN      8    // LEDs por bloque de color antes de cambiar
#define EROSION_INTERVAL 4    // spawns del mismo jugador antes de borrar su LED más viejo
#define SHIFT_STEPS      10   // LEDs desplazados hacia el oponente en un match
#define SHIFT_STEP_MS    20   // ms por paso del desplazamiento (10 pasos = 200ms)

// Sonidos del juego (formato RTTTL)
#define SND_SHOOT  "Double:d=16,o=5,b=200:c6,f6,c7"

struct Shot {
  int  pos;
  int  dir;
  bool active;
  CRGB color;
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
  int           spawnIntervalMs; // intervalo actual (decrece con SPAWN_ACCEL)
  int           spawnTurn;       // 0=izquierda, 1=derecha
  int           leftLedInSeg;
  int           rightLedInSeg;
  int           leftSpawnCount;  // total de LEDs spawneados en lado izquierdo
  int           rightSpawnCount;
  int           leftColorIdx;
  int           rightColorIdx;
  unsigned long lastSpawn;

  int           shiftRemaining;
  int           shiftDir;
  unsigned long lastShift;

  bool          gameOver;
  int           loser;

  static const CRGB COLORS[3];

  void fire(int player);
  void moveShots();
  void checkCollisions();
  void destroySegAt(int hitIdx, int minBound, int maxBound);
  void spawnNextLed();
  void addPenalty(int player, CRGB color);
  void startShift(int dir);
  void stepShift();
  void erodeLeft();
  void erodeRight();
  bool checkGameOver();
  void renderFrame();
  void renderSpawner();
  void renderGameOver();
  void updateDisplay();
  int  findFreeSlot();
  int  pickOtherColor(int current);
  int  nextColorInCycle(int current);
};
