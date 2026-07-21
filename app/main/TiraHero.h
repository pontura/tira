#pragma once
#include "GameBase.h"

#define TH_LEDS_PER_EIGHTH  4     // LEDs que representa una corchea
#define TH_BASE_SIZE        1     // ancho de la base de cada player (= 1 corchea)
#define TH_MAX_NOTES       128
#define TH_MAX_BLOCKS       80    // bloques activos simultáneos (2 por nota: L + R)
#define TH_UPDATE_MS        16
#define TH_BASE_OFFSET     15     // LEDs desde el centro hasta la base de cada player
#define TH_HIT_OFFSET       3     // tolerancia en LEDs para hit/release
#define TH_MAX_SPARKS      48     // chispas simultáneas máximas

struct TH_Note {
  uint16_t freq;       // Hz (0 = silencio)
  int      size;       // LEDs del bloque
  uint32_t spawnMs;    // ms desde songStartMs para spawear
  uint32_t durMs;      // duración real del sonido en ms
};

struct TH_Spark {   // 12 bytes (vs 32 original)
  float   pos;      // posición actual (float para movimiento suave)
  float   vel;      // LEDs por ms
  uint8_t maxDist;  // distancia máxima en LEDs (4-16)
  int8_t  dir;      // +1 o -1
  uint8_t player;   // 0=P1, 1=P2
  uint8_t flags;    // bit0: active  bit1: isGreen
};

struct TH_Block {
  float    leftEdge;
  int      dir;        // +1 = derecha, -1 = izquierda
  int      size;
  CRGB     color;
  uint16_t freq;
  uint32_t durMs;
  bool     active;
  bool     fired;
  bool     visible;   // false = suena pero no se dibuja
  bool     missed;    // true = llegó a la base sin ser presionada → roja, sin sonido
  bool     hit;       // true = el player la presionó a tiempo (persiste aunque suelte el botón)
};

class TiraHero : public GameBase {
public:
  TiraHero(CRGB* leds, int numLeds, U8G2* display);
  void begin()   override;
  void update()  override;
  void onInput(int player, int button) override;
  void onButtonUp(int player, int button) override;
  void onAnalog(int player, int16_t tiltX, int16_t tiltY) override {}

private:
  TH_Note       notes[TH_MAX_NOTES];
  int           noteCount;
  TH_Block      blocks[TH_MAX_BLOCKS];
  TH_Spark      sparks[TH_MAX_SPARKS];

  float         speed;        // LEDs por ms
  float         quarterMs;   // duración de una negra en ms
  uint32_t      travelMs;    // tiempo de viaje centro→base (ms)
  unsigned long songStartMs;
  int           songIdx;
  int           nextNote;
  unsigned long lastUpdate;
  unsigned long melodyEndMs;   // hasta cuándo está sonando una nota de melodía
  uint16_t      lastMelodyFreq; // frecuencia de la última nota de melodía
  int           lastBeatNum;  // último beat de negra procesado

  // Estado de hit por player (índice 0=P1, 1=P2)
  bool buttonHeld[2];     // estado físico del botón (true = apretado)
  bool badHold[2];        // botón abajo sin nota en base → base roja
  bool goodHold[2];       // botón abajo con nota correcta en base → base verde
  int           activeHitBlock[2]; // índice en blocks[] del bloque siendo sostenido
  int           penaltyCount[2];   // errores acumulados por player (0-3)
  bool          playerOut[2];      // true = eliminado (3 errores)
  bool          gameOver;          // ambos eliminados, esperando restart
  unsigned long endMs;             // millis en que termina la pantalla roja de game over

  void     parseRTTTL(const char* rtttl);
  int      findFreeBlock();
  void     spawnNote(int ni);
  void     addPenalty(int p);
  void     renderFrame();
};
