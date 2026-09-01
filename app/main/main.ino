#include <FastLED.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "SoundManager.h"
#include "MainMenu.h"
#include "TiraTenis.h"
#include "TiraZombies.h"
#include "TiraColors.h"
#include "TiraHero.h"
#include "TiraPaint.h"
#include "Left2Dead.h"
#include "Metegol.h"

#define LED_PIN     25
#define NUM_LEDS    144
#define BRIGHTNESS  50
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define GAME_COUNT  7
#define MENU_RETURN_MS 2000   // mantener SW 2s en-juego para volver al menú
#define JOYSTICK_TIMEOUT_MS 2000  // sin paquetes en este lapso = joystick apagado/desconectado

// Dirección de broadcast para sonidos a todos los joysticks
uint8_t broadcastMAC[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// MACs aprendidas automáticamente cuando cada joystick envía su primer paquete
uint8_t joystickMAC[2][6]    = {{0},{0}};
bool    joystickKnown[2]      = {false, false};
bool    pendingRegister[2]    = {false, false};
uint8_t pendingMAC[2][6]      = {{0},{0}};
volatile unsigned long joystickLastSeen[2] = {0, 0};  // millis() del último paquete recibido de cada joystick

// true si el joystick de ese player mandó algo en los últimos JOYSTICK_TIMEOUT_MS
bool isPlayerConnected(int player) {
  int idx = player - 1;
  if (idx < 0 || idx > 1) return false;
  return joystickKnown[idx] && (millis() - joystickLastSeen[idx] < JOYSTICK_TIMEOUT_MS);
}

struct JoyInputPacket {
  uint8_t playerId;
  uint8_t button;
  uint8_t pressed;  // 1=presionado, 0=soltado
};

struct JoyAnalogPacket {
  uint8_t playerId;
  int16_t tiltX;
  int16_t tiltY;
  int16_t tiltZ;
};

struct MasterSoundPacket {
  char rtttl[64];
};

CRGB leds[NUM_LEDS];
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
bool displayOK = false;

const char* gameNames[GAME_COUNT] = {
  "Left2Dead", "TiraTenis", "TiraZombies",
  "TiraColors", "TiraHero", "TiraPaint",
  "Metegol"
};
MainMenu    mainMenu(&display, gameNames, GAME_COUNT, leds, NUM_LEDS);
GameBase*   currentGame = nullptr;
int         currentGameIdx = -1;

// Estado del botón SW del joystick local para volver al menú
bool          swHeld    = false;
unsigned long swHeldMs  = 0;

void returnToMenu() {
  SoundManager::stop();  // ya corta el tono (rtttl::noTone internamente)
  delete currentGame;
  currentGame    = nullptr;
  currentGameIdx = -1;
  swHeld         = false;
  mainMenu.begin();
}

void switchGame(int idx) {
  SoundManager::stop();  // ya corta el tono (rtttl::noTone internamente)
  delete currentGame;
  currentGame    = nullptr;
  currentGameIdx = idx;

  switch (idx) {
    case 0: currentGame = new Left2Dead(leds, NUM_LEDS, &display);   break;
    case 1: currentGame = new TiraTenis(leds, NUM_LEDS, &display);   break;
    case 2: currentGame = new TiraZombies(leds, NUM_LEDS, &display); break;
    case 3: currentGame = new TiraColors(leds, NUM_LEDS, &display);  break;
    case 4: currentGame = new TiraHero(leds, NUM_LEDS, &display);    break;
    case 5: currentGame = new TiraPaint(leds, NUM_LEDS, &display);   break;
    case 6: currentGame = new Metegol(leds, NUM_LEDS, &display);     break;
  }
  if (displayOK) {
    char title[24];
    sprintf(title, "< %s", gameNames[idx]);
    display.clearBuffer();
    display.setFont(u8g2_font_7x14B_tr);
    display.setDrawColor(1);
    display.drawBox(0, 0, 128, 16);
    display.setDrawColor(0);
    display.drawStr(4, 13, title);
    display.setDrawColor(1);
    display.sendBuffer();
    delay(800);
  }
  currentGame->begin();
}

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
      peer.channel = 1;
      peer.encrypt = false;
      peer.ifidx = WIFI_IF_STA;
      esp_now_add_peer(&peer);
      joystickKnown[i] = true;
    }
  }
}

volatile bool    joyPending  = false;
volatile uint8_t joyPlayer   = 0;
volatile uint8_t joyButton   = 0;
volatile uint8_t joyPressed  = 1;

volatile bool    analogPending = false;
volatile uint8_t analogPlayer  = 0;
volatile int16_t analogTiltX   = 0;
volatile int16_t analogTiltY   = 0;
volatile int16_t analogTiltZ   = 0;

void onJoystickData(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (len == sizeof(JoyAnalogPacket)) {
    const JoyAnalogPacket* pkt = (const JoyAnalogPacket*)data;
    int idx = pkt->playerId - 1;
    if (idx >= 0 && idx < 2) joystickLastSeen[idx] = millis();
    if (idx >= 0 && idx < 2 && !joystickKnown[idx] && !pendingRegister[idx]) {
      Serial.printf("[ESP-NOW] Joystick %d conectado\n", pkt->playerId);
      memcpy(pendingMAC[idx], info->src_addr, 6);
      pendingRegister[idx] = true;
    }
    analogPlayer  = pkt->playerId;
    analogTiltX   = pkt->tiltX;
    analogTiltY   = pkt->tiltY;
    analogTiltZ   = pkt->tiltZ;
    analogPending = true;
    return;
  }
  if (len != sizeof(JoyInputPacket)) {
    Serial.printf("[ESP-NOW] pkt desconocido len=%d (analog=%d input=%d)\n",
                  len, (int)sizeof(JoyAnalogPacket), (int)sizeof(JoyInputPacket));
    return;
  }
  const JoyInputPacket* pkt = (const JoyInputPacket*)data;

  int idx = pkt->playerId - 1;
  if (idx >= 0 && idx < 2) joystickLastSeen[idx] = millis();
  if (idx >= 0 && idx < 2 && !joystickKnown[idx] && !pendingRegister[idx]) {
    memcpy(pendingMAC[idx], info->src_addr, 6);
    pendingRegister[idx] = true;
  }

  Serial.printf("[BTN] p=%d b=%d %s\n", pkt->playerId, pkt->button, pkt->pressed ? "DOWN" : "UP");
  joyPlayer  = pkt->playerId;
  joyButton  = pkt->button;
  joyPressed = pkt->pressed;
  joyPending = true;
}

void setup() {
  Serial.begin(115200);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(300);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  Serial.print("Master MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] init FAILED");
    return;
  }
  Serial.println("[ESP-NOW] init OK");
  if (esp_now_register_recv_cb(onJoystickData) != ESP_OK) {
    Serial.println("[ESP-NOW] register_recv_cb FAILED");
    return;
  }
  Serial.println("[ESP-NOW] callback OK");
  Serial.printf("Canal confirmado: %d\n", WiFi.channel());

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcastMAC, 6);
  peer.channel = 1;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_STA;
  esp_now_add_peer(&peer);

  SoundManager::begin();
  SoundManager::setOnPlayCallback(sendSoundToJoysticks);
  SoundManager::setOnJoystickCallback(sendSoundToJoysticks);
  SoundManager::setOnPlayerCallback(sendSoundToPlayer);

  displayOK = display.begin();
  if (displayOK) display.setContrast(255);
  Serial.printf("[DISPLAY] %s\n", displayOK ? "OK" : "no responde — desactivada");

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  mainMenu.begin();
}

void loop() {
  registerPendingJoysticks();

  if (currentGame == nullptr) {
    // ── En menú principal ──────────────────────────────────────────────
    int toLaunch = mainMenu.update();
    if (toLaunch >= 0) switchGame(toLaunch);
  } else {
    // ── En juego ──────────────────────────────────────────────────────
    // Joystick izquierda (VRX) → volver al menú
    if ((MENU_CENTER - analogRead(MENU_VRX_PIN)) > MENU_DEADZONE) {
      returnToMenu();
      return;
    }

    // SW local: mantener 2s para volver al menú (fallback)
    bool sw = !digitalRead(MENU_SW_PIN);
    if (sw && !swHeld) { swHeld = true; swHeldMs = millis(); }
    if (!sw)             swHeld = false;
    if (swHeld && millis() - swHeldMs >= MENU_RETURN_MS) returnToMenu();

    if (analogPending) {
      analogPending = false;
      currentGame->onAnalog(analogPlayer, analogTiltX, analogTiltY, analogTiltZ);
    }
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
}
