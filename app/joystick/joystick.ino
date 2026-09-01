#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <NonBlockingRtttl.h>

// Descomentar para modo debug standalone (sin master, solo Serial)
// #define STANDALONE_DEBUG

// ── Configuración del joystick ──────────────────────────────────────
#define PLAYER_ID    2    // Cambiar a 2 en el segundo joystick
#define BTN_FIRE     8    // GPIO8 — botón disparar (INPUT_PULLUP, activo LOW)
#define BTN_COLOR    10   // GPIO10 — botón cambiar color
#define BUZZER_PIN   7    // GPIO7 — buzzer pasivo

uint8_t masterMAC[] = { 0xE0, 0x8C, 0xFE, 0x5C, 0x42, 0x94 };

// ── Estructuras de paquetes (deben coincidir con main.ino) ───────────
struct JoyInputPacket {
  uint8_t playerId;
  uint8_t button;    // 0=disparar, 1=cambiar color
  uint8_t pressed;   // 1=presionado, 0=soltado
};

struct MasterSoundPacket {
  char rtttl[64];
};

struct JoyAnalogPacket {
  uint8_t playerId;
  int16_t tiltX;
  int16_t tiltY;
  int16_t tiltZ;
};

// ── MPU-6050 ─────────────────────────────────────────────────────────
#define MPU_ADDR     0x68
#define MPU_SDA      3
#define MPU_SCL      4
#define ANALOG_MS    32

unsigned long lastAnalogMs = 0;

void mpuInit() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);  // PWR_MGMT_1
  Wire.write(0x00);  // wake up
  Wire.endTransmission(true);
}

void sendAnalog() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);  // ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);
  int16_t ax = (int16_t)((Wire.read() << 8) | Wire.read());
  int16_t ay = (int16_t)((Wire.read() << 8) | Wire.read());
  int16_t az = (int16_t)((Wire.read() << 8) | Wire.read());

  JoyAnalogPacket pkt = { PLAYER_ID, ax, ay, az };
  esp_now_send(masterMAC, (uint8_t*)&pkt, sizeof(pkt));
}

// ── Estado de botones ────────────────────────────────────────────────
#define DEBOUNCE_MS      15
const int btnPins[2]          = { BTN_FIRE, BTN_COLOR };
bool      btnState[2]         = { false, false };
unsigned long lastChangeMs[2] = { 0, 0 };

// ── ESP-NOW callbacks ────────────────────────────────────────────────

void onDataRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  Serial.printf("[RX] len=%d src=%02X:%02X:%02X:%02X:%02X:%02X\n", len,
    info->src_addr[0], info->src_addr[1], info->src_addr[2],
    info->src_addr[3], info->src_addr[4], info->src_addr[5]);
  if (len != sizeof(MasterSoundPacket)) return;
  MasterSoundPacket* pkt = (MasterSoundPacket*)data;
  rtttl::begin(BUZZER_PIN, pkt->rtttl);
}

void onDataSent(const wifi_tx_info_t* info, esp_now_send_status_t status) {
  static int count = 0;
  if (++count <= 3)
    Serial.printf("[TX] #%d %s\n", count, status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
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

  // Test buzzer
  tone(BUZZER_PIN, 1000, 200); delay(300);
  tone(BUZZER_PIN, 2000, 200); delay(300);

  Serial.println("\n=== JOYSTICK DEBUG ===");
  Serial.printf("SDA=GPIO%d  SCL=GPIO%d  BUZZER=GPIO%d  BTN1=GPIO%d  BTN2=GPIO%d\n",
                MPU_SDA, MPU_SCL, BUZZER_PIN, BTN_FIRE, BTN_COLOR);

  Wire.begin(MPU_SDA, MPU_SCL);

  // Detectar MPU
  Wire.beginTransmission(MPU_ADDR);
  uint8_t err = Wire.endTransmission();
  if (err == 0) {
    Serial.println("[MPU] Detectado OK en 0x68");
    mpuInit();
  } else {
    Serial.printf("[MPU] ERROR — no encontrado (err=%d). Revisá SDA/SCL.\n", err);
    // beep de error
    for (int i = 0; i < 3; i++) { tone(BUZZER_PIN, 300, 150); delay(300); }
  }

#ifndef STANDALONE_DEBUG
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(300);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  Serial.printf("Joystick %d MAC: %s  canal: %d\n",
    PLAYER_ID, WiFi.macAddress().c_str(), WiFi.channel());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] init FAILED");
  } else {
    esp_now_register_recv_cb(onDataRecv);
    esp_now_register_send_cb(onDataSent);
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, masterMAC, 6);
    peer.channel = 1;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
    Serial.println("Listo.");
  }
#else
  Serial.println("[MODO DEBUG] Sin WiFi/ESP-NOW. Solo Serial.");
  Serial.println("Inclina el control — BTN1=disparar  BTN2=color");
  Serial.println("ax       ay       tilt");
#endif
}

// ── Loop ─────────────────────────────────────────────────────────────

#ifdef STANDALONE_DEBUG

void loop() {
  unsigned long now = millis();

  // Leer MPU y mostrar cada 150ms
  if (now - lastAnalogMs >= 150) {
    lastAnalogMs = now;

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 4, true);
    int16_t ax = (int16_t)((Wire.read() << 8) | Wire.read());
    int16_t ay = (int16_t)((Wire.read() << 8) | Wire.read());

    // Barra visual de tilt: <<<<< centro >>>>>
    const int DEAD = 2000;
    char bar[22]; memset(bar, ' ', 21); bar[21] = '\0';
    bar[10] = '|';
    if (abs(ax) > DEAD) {
      int pos = constrain(map(ax, -16384, 16384, 0, 20), 0, 20);
      bar[pos] = (ax < 0) ? '<' : '>';
    }

    Serial.printf("%7d  %7d  [%s]\n", ax, ay, bar);
  }

  // Botones
  for (int b = 0; b < 2; b++) {
    bool isPressed = !digitalRead(btnPins[b]);
    if (isPressed != btnState[b] && now - lastChangeMs[b] > DEBOUNCE_MS) {
      lastChangeMs[b] = now;
      btnState[b]     = isPressed;
      Serial.printf("[BTN%d] %s\n", b + 1, isPressed ? "PRESS" : "RELEASE");
      if (isPressed) tone(BUZZER_PIN, b == 0 ? 1500 : 1000, 80);
    }
  }
}

#else  // modo normal con WiFi

void loop() {
  rtttl::play();

  unsigned long now = millis();

  if (now - lastAnalogMs >= ANALOG_MS) {
    lastAnalogMs = now;
    sendAnalog();
  }

  for (int b = 0; b < 2; b++) {
    bool isPressed = !digitalRead(btnPins[b]);

    if (isPressed != btnState[b] && now - lastChangeMs[b] > DEBOUNCE_MS) {
      lastChangeMs[b] = now;
      btnState[b]     = isPressed;
      sendInput(b, isPressed ? 1 : 0);
    }
  }
}

#endif
