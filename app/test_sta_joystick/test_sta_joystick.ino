#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

uint8_t masterMAC[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };  // broadcast

void onSent(const wifi_tx_info_t* info, esp_now_send_status_t status) {
  static int count = 0;
  if (++count <= 5)
    Serial.printf("[TX] #%d %s\n", count, status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(300);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  Serial.printf("Joystick STA MAC: %s  canal: %d\n", WiFi.macAddress().c_str(), WiFi.channel());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW FAIL");
    return;
  }
  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, masterMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  Serial.println("Enviando...");
}

void loop() {
  static unsigned long last = 0;
  if (millis() - last >= 500) {
    last = millis();
    uint8_t data[] = { 0x01, 0x02, 0x03 };
    esp_now_send(masterMAC, data, sizeof(data));
  }
}
