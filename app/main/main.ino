#include <FastLED.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include "SoundManager.h"
#include "TiraMatch.h"
#include "TiraTenis.h"

#define LED_PIN     5
#define NUM_LEDS    288
#define BRIGHTNESS  50
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

// Dirección de broadcast para sonidos a todos los joysticks
uint8_t broadcastMAC[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// MACs aprendidas automáticamente cuando cada joystick envía su primer paquete
uint8_t joystickMAC[2][6]    = {{0},{0}};
bool    joystickKnown[2]      = {false, false};
bool    pendingRegister[2]    = {false, false};
uint8_t pendingMAC[2][6]      = {{0},{0}};

struct JoyInputPacket {
  uint8_t playerId;
  uint8_t button;
  uint8_t pressed;  // 1=presionado, 0=soltado
};

struct MasterSoundPacket {
  char rtttl[64];
};

CRGB leds[NUM_LEDS];
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
GameBase* currentGame;

// Envía sonido a todos los joysticks
void sendSoundToJoysticks(const char* rtttl) {
  MasterSoundPacket pkt;
  strncpy(pkt.rtttl, rtttl, sizeof(pkt.rtttl) - 1);
  pkt.rtttl[sizeof(pkt.rtttl) - 1] = '\0';
  esp_now_send(broadcastMAC, (uint8_t*)&pkt, sizeof(pkt));
}

// Envía sonido solo al joystick del jugador indicado
void sendSoundToPlayer(uint8_t player, const char* rtttl) {
  int idx = player - 1;
  if (idx < 0 || idx > 1 || !joystickKnown[idx]) return;
  MasterSoundPacket pkt;
  strncpy(pkt.rtttl, rtttl, sizeof(pkt.rtttl) - 1);
  pkt.rtttl[sizeof(pkt.rtttl) - 1] = '\0';
  esp_now_send(joystickMAC[idx], (uint8_t*)&pkt, sizeof(pkt));
}

// Registra peers aprendidos desde el loop principal (no desde el callback)
void registerPendingJoysticks() {
  for (int i = 0; i < 2; i++) {
    if (pendingRegister[i] && !joystickKnown[i]) {
      pendingRegister[i] = false;
      memcpy(joystickMAC[i], pendingMAC[i], 6);
      esp_now_peer_info_t peer = {};
      memcpy(peer.peer_addr, joystickMAC[i], 6);
      peer.channel = 0;
      peer.encrypt = false;
      esp_now_add_peer(&peer);
      joystickKnown[i] = true;
    }
  }
}

volatile bool    joyPending  = false;
volatile uint8_t joyPlayer   = 0;
volatile uint8_t joyButton   = 0;
volatile uint8_t joyPressed  = 1;

void onJoystickData(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (len != sizeof(JoyInputPacket)) return;
  const JoyInputPacket* pkt = (const JoyInputPacket*)data;

  int idx = pkt->playerId - 1;
  if (idx >= 0 && idx < 2 && !joystickKnown[idx] && !pendingRegister[idx]) {
    memcpy(pendingMAC[idx], info->src_addr, 6);
    pendingRegister[idx] = true;
  }

  joyPlayer  = pkt->playerId;
  joyButton  = pkt->button;
  joyPressed = pkt->pressed;
  joyPending = true;
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  Serial.print("Master MAC: ");
  Serial.println(WiFi.macAddress());

  esp_now_init();
  esp_now_register_recv_cb(onJoystickData);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcastMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  SoundManager::begin();
  SoundManager::setOnPlayCallback(sendSoundToJoysticks);
  SoundManager::setOnJoystickCallback(sendSoundToJoysticks);
  SoundManager::setOnPlayerCallback(sendSoundToPlayer);

  display.begin();

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  currentGame = new TiraTenis(leds, NUM_LEDS, &display);
  currentGame->begin();
}

void loop() {
  registerPendingJoysticks();

  if (joyPending) {
    joyPending = false;
    if (joyPressed)
      currentGame->onInput(joyPlayer, joyButton);
    else
      currentGame->onButtonUp(joyPlayer, joyButton);
  }
  SoundManager::update();
  currentGame->update();
}
