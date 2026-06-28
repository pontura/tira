#include <esp_now.h>
#include <WiFi.h>
#include <NonBlockingRtttl.h>

// ── Configuración del joystick ──────────────────────────────────────
#define PLAYER_ID    1    // Cambiar a 2 en el segundo joystick
#define BTN_FIRE     7    // GPIO7 — botón disparar (INPUT_PULLUP, activo LOW)
#define BTN_COLOR    9    // GPIO9 — botón cambiar color
#define BUZZER_PIN   4    // GPIO4 — buzzer pasivo

// MAC del master — copiar lo que imprime el master en Serial al arrancar
uint8_t masterMAC[] = { 0xC4, 0xDD, 0x57, 0x93, 0x35, 0xF0 };

// ── Estructuras de paquetes (deben coincidir con main.ino) ───────────
struct JoyInputPacket {
  uint8_t playerId;
  uint8_t button;    // 0=disparar, 1=cambiar color
  uint8_t pressed;   // 1=presionado, 0=soltado
};

struct MasterSoundPacket {
  char rtttl[64];
};

// ── Estado de botones ────────────────────────────────────────────────
#define DEBOUNCE_MS 15
const int btnPins[2]          = { BTN_FIRE, BTN_COLOR };
bool      btnState[2]         = { false, false };
unsigned long lastChangeMs[2] = { 0, 0 };

// ── ESP-NOW callbacks ────────────────────────────────────────────────

void onDataRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (len != sizeof(MasterSoundPacket)) return;
  MasterSoundPacket* pkt = (MasterSoundPacket*)data;
  rtttl::begin(BUZZER_PIN, pkt->rtttl);
}

void sendInput(uint8_t button, uint8_t pressed) {
  JoyInputPacket pkt = { PLAYER_ID, button, pressed };
  esp_now_send(masterMAC, (uint8_t*)&pkt, sizeof(pkt));
}

// ── Setup ────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);

  pinMode(BTN_FIRE,  INPUT_PULLUP);
  pinMode(BTN_COLOR, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  // Test buzzer antes de WiFi
  tone(BUZZER_PIN, 1000, 400);
  delay(600);
  tone(BUZZER_PIN, 500, 400);
  delay(600);

  WiFi.mode(WIFI_STA);
  Serial.print("Joystick ");
  Serial.print(PLAYER_ID);
  Serial.print(" MAC: ");
  Serial.println(WiFi.macAddress());

  esp_now_init();
  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, masterMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  Serial.println("Listo. Actualiza masterMAC con la MAC del master.");
}

// ── Loop ─────────────────────────────────────────────────────────────

void loop() {
  rtttl::play();

  unsigned long now = millis();

  for (int b = 0; b < 2; b++) {
    bool isPressed = !digitalRead(btnPins[b]);

    if (isPressed != btnState[b] && now - lastChangeMs[b] > DEBOUNCE_MS) {
      lastChangeMs[b] = now;
      btnState[b]     = isPressed;
      sendInput(b, isPressed ? 1 : 0);
    }
  }
}
