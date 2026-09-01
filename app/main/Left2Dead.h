#pragma once
#include "GameBase.h"

#define L2D_ANGLE_MAX_DEG  45.0f  // inclinación (°) a la que la posición ya está en el extremo
#define L2D_POS_SMOOTHING  0.12f  // filtro exponencial (0..1): más bajo = más suave, evita que la mira salte entre LEDs vecinos
#define L2D_FIRE_COOLDOWN_MS 200  // tiempo mínimo entre disparos de un mismo player (máx. 5 disparos/seg)
#define L2D_UPDATE_MS        16   // tope de framerate (~60fps); evita renderizar/mostrar la tira más seguido de lo necesario

// ── Flash blanco al disparar (5 → 3 → 1 LEDs, centrado en la mira) ──────
#define L2D_SHOT_FLASH_S1_MS  100  // hasta acá: 5 LEDs
#define L2D_SHOT_FLASH_S2_MS  200  // hasta acá: 3 LEDs (100ms más)
#define L2D_SHOT_FLASH_S3_MS  400  // hasta acá: 1 LED (200ms más); después, nada

// ── Enemigos (zombies) ──────────────────────────────────────────────────
#define L2D_MAX_ENEMIES          6     // zombies simultáneos máximos
#define L2D_ENEMY_SPAWN_START_MS 3000  // intervalo de spawn inicial
#define L2D_ENEMY_SPAWN_MIN_MS    500  // intervalo de spawn mínimo (techo de dificultad)
#define L2D_ENEMY_SPAWN_STEP_MS   200  // cuánto baja el intervalo en cada spawn (3.0 → 2.8 → 2.6 → ...)
#define L2D_ENEMY_GROW_START_MS  1000  // ms por momento al inicio (cuando el spawn está en 3.0s)
#define L2D_ENEMY_GROW_MIN_MS     500  // ms por momento en el punto más difícil (rango angosto = incremento gradual)
#define L2D_ENEMY_MIN_LED         26   // rango de spawn/destino: >25
#define L2D_ENEMY_MAX_LED        118   // rango de spawn/destino: <119
#define L2D_ENEMY_STAGES          10   // cantidad de momentos definidos (tamaño máximo)

// ── Modos de juego ────────────────────────────────────────────────────
// Modo 1: zombies quietos (como hasta ahora). Modo 2: zombies que salen desde
// cada extremo de la tira y avanzan hacia un punto al azar, en cámara lenta al
// final, llegando a destino justo cuando terminan de crecer (se ponen rojos).
// Cada modo dura L2D_MODE_DURATION_MS de spawn; agotado ese tiempo, deja de
// aparecer gente nueva y, cuando se limpia la pantalla, se pasa al otro modo
// con la dificultad reseteada a mitad de camino entre el reset anterior y la actual.
#define L2D_MODE_DURATION_MS  40000

// ── Explosión de partículas al matar un zombie ──────────────────────────
#define L2D_MAX_PARTICLES   16     // partículas simultáneas máximas
#define L2D_PARTICLE_DECEL  0.85f  // factor de desaceleración por frame (menor = frena más rápido)
#define L2D_PARTICLE_MS     16     // ms entre actualizaciones de partículas (~60fps)

// ── Advertencia: zombie llegó al momento máximo, último chance de matarlo ─
#define L2D_WARNING_MS       500  // gracia antes de atacar: sigue jugándose, todo rojo + fondo tenue
#define L2D_WARNING_BG_R       5  // brillo (~2%) del fondo rojo tenue durante la advertencia

// ── Secuencia de fin de partida: un zombie ataca ────────────────────────
#define L2D_FREEZE_MS          2000  // el rojo del zombie atacante se disemina por toda la tira
#define L2D_FREEZE_BEEP_MS      200  // cada cuánto suena una nota al azar de la caída descendente
#define L2D_TALLY_STEP_MS       300  // ms entre cada LED del conteo de muertes
#define L2D_TALLY_END_WAIT_MS  1000  // pausa final antes de reiniciar el juego

// ── Alerta: mira sobre un enemigo ───────────────────────────────────────
#define L2D_ALERT_BEEP_MS  350  // cada cuánto se repite el beep mientras la mira sigue enganchada

// ── Jugadores conectados ─────────────────────────────────────────────────
// Si al entrar no hay ningún control prendido, el juego espera. Con uno solo
// conectado, se dibuja únicamente ese player y los zombies aparecen a la
// mitad de ritmo (intervalo x2); en cuanto se conecta el segundo, pasa a
// ritmo normal y se dibujan ambos.
#define L2D_SOLO_SPAWN_MULT  2

#define SND_L2D_SHOT  "Shot:d=16,o=5,b=200:c6"
#define SND_L2D_HIT   "Hit2:d=8,o=5,b=200:c6,g5,c5"
#define SND_L2D_ZBOOM "Zb:d=32,o=2,b=280:c,e,g,a#,c3,a#,g,e"   // explosión corta al matar un zombie: ráfaga grave rápida
#define SND_L2D_ALERT "Alt:d=32,o=6,b=300:c"                   // beep corto y agudo: mira enganchada a un enemigo

struct L2D_Enemy {
  int           centerLed;  // modo 1: LED fijo donde apareció (no se mueve)
  int           stage;      // 1..L2D_ENEMY_STAGES
  unsigned long lastGrow;
  unsigned long spawnTime;    // millis() al aparecer; los más viejos ganan dibujo y colisión
  bool          warning;      // true = llegó al momento máximo, en gracia antes de atacar (todo rojo)
  unsigned long warningUntil; // millis() en que se acaba la gracia y ataca si sigue vivo
  bool          active;

  bool          moving;     // modo 2: viaja de originLed a targetLed en vez de estar quieto
  int           originLed;  // LED de arranque (un extremo de la tira)
  int           targetLed;  // LED de destino (al azar), llega justo al terminar de crecer
};

struct L2D_Particle {
  float pos;     // posición actual (float para movimiento suave)
  float vel;     // velocidad actual (LEDs por frame)
  float maxVel;  // velocidad inicial (para calcular brillo)
  int   dir;     // -1 o +1
  bool  active;
};

enum L2D_Phase {
  L2D_WAITING,     // esperando que se conecte al menos un player
  L2D_PLAYING,     // juego normal
  L2D_FREEZE,      // rojo del zombie atacante diseminándose por toda la tira
  L2D_TALLY,       // revelando el conteo de muertes, un LED a la vez
  L2D_TALLY_WAIT   // conteo completo, pausa antes de reiniciar
};

class Left2Dead : public GameBase {
public:
  Left2Dead(CRGB* leds, int numLeds, U8G2* display);
  void begin()  override;
  void update() override;
  void onInput(int player, int button) override;
  void onAnalog(int player, int16_t tiltX, int16_t tiltY, int16_t tiltZ) override;

private:
  float         pos[2];         // posición absoluta de cada player (float, LEDs), toda la tira
  int           kills[2];       // zombies eliminados por cada player en esta partida
  unsigned long lastFireTime[2];  // millis() del último disparo válido de cada player
  bool          playerActive[2];  // true una vez que ese player se conectó (ver checkPlayers)

  L2D_Enemy     enemies[L2D_MAX_ENEMIES];
  unsigned long lastEnemySpawn;
  float         enemySpawnIntervalMs;  // baja de L2D_ENEMY_SPAWN_START_MS a L2D_ENEMY_SPAWN_MIN_MS
  int           nextSpawnSide;         // alterna según el modo (ver spawnEnemies)

  int           gameMode;              // 1 o 2
  unsigned long modeStartMs;           // millis() en que arrancó a spawnear el modo actual
  bool          modeSpawningOff;       // true = se cumplieron los 40s, esperando limpiar pantalla
  float         lastModeResetInterval; // dificultad fijada en el último cambio de modo (o la inicial)

  L2D_Particle  particles[L2D_MAX_PARTICLES];
  unsigned long lastParticleUpdate;

  unsigned long lastUpdate;  // tope de framerate de update()/renderFrame()

  unsigned long shotFlashStart[2];   // millis() del último disparo de cada player
  int           shotFlashCenter[2];  // LED donde se centra el flash, -1 = sin flash activo

  bool          alertActive[2];    // true si la mira está enganchada a un enemigo ahora mismo
  unsigned long lastAlertBeep[2];  // millis() del último beep de alerta enviado

  // Secuencia de fin de partida
  L2D_Phase     phase;
  unsigned long phaseUntil;      // usado por L2D_FREEZE y L2D_TALLY_WAIT
  int           freezeStart;     // LED donde arranca el zombie atacante
  int           freezeLen;       // ancho del zombie atacante
  unsigned long freezeAnimStart; // millis() al empezar a diseminarse el rojo
  unsigned long lastFreezeBeep;  // millis() de la última nota de la caída descendente
  char          freezeBeepBuf[32];
  int           tallyShown[2];   // LEDs de muertes ya revelados por player, en L2D_TALLY
  int           tallyTurn;       // a quién le toca revelar el próximo LED (0=P1, 1=P2)
  unsigned long lastTallyStep;

  void fire(int player);
  int  findEnemyAt(int led);   // índice del zombie que ocupa ese LED, o -1
  int  enemyPos(const L2D_Enemy& z);  // posición actual (fija en modo 1, animada en modo 2)
  unsigned long currentGrowMs();      // ms por momento, según la dificultad actual
  void updateAlert();
  void checkPlayers();  // activa players a medida que se conectan; arranca el juego con el primero

  int  findFreeEnemy();
  void spawnEnemies();
  void growEnemies();
  void checkModeTransition();
  void startFreeze(int start, int len);
  void updateFreezeSpread();
  void updateTally();

  void spawnZombieExplosion(int centerLed, int len);
  void updateParticles();
  void renderParticles();
  int  findFreeParticle();

  void updateDisplay();
  void renderFrame();
};
