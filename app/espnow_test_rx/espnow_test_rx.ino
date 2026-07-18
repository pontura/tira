#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// MAC STA del joystick
const uint8_t joyMAC[] = { 0x44, 0xBD, 0x8D, 0x24, 0x9E, 0x8C };

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t* p = (wifi_promiscuous_pkt_t*)buf;
  if (p->rx_ctrl.sig_len < 16) return;
  uint8_t* ta = p->payload + 10;
  if (memcmp(ta, joyMAC, 6) == 0) {
    Serial.printf("[JOYSTICK] type=%d rssi=%d len=%d\n",
      type, p->rx_ctrl.rssi, p->rx_ctrl.sig_len);
  }
}

void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  Serial.printf("[RX] len=%d  src=%02X:%02X:%02X:%02X:%02X:%02X\n", len,
    info->src_addr[0], info->src_addr[1], info->src_addr[2],
    info->src_addr[3], info->src_addr[4], info->src_addr[5]);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(300);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  Serial.printf("STA MAC: %s  canal: %d\n",
    WiFi.macAddress().c_str(), WiFi.channel());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW FAIL");
    return;
  }
  esp_now_register_recv_cb(onRecv);

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(sniffer);
  Serial.println("Escuchando...");
}

void loop() {
  static unsigned long last = 0;
  if (millis() - last >= 2000) {
    last = millis();
    Serial.println("[heartbeat]");
  }
}
