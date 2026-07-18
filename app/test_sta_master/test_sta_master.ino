#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// MAC del joystick en modo STA
const uint8_t joyMAC[] = { 0x44, 0xBD, 0x8D, 0x24, 0x87, 0x4C };

void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  Serial.printf("[RX] len=%d  src=%02X:%02X:%02X:%02X:%02X:%02X\n", len,
    info->src_addr[0], info->src_addr[1], info->src_addr[2],
    info->src_addr[3], info->src_addr[4], info->src_addr[5]);
}

// Sniffer promiscuo: imprime solo frames cuya TA (bytes 10-15) es el joystick
void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t* p = (wifi_promiscuous_pkt_t*)buf;
  if (p->rx_ctrl.sig_len < 16) return;
  if (memcmp(p->payload + 10, joyMAC, 6) == 0) {
    Serial.printf("[SNIFF] type=%d len=%d rssi=%d\n",
      type, p->rx_ctrl.sig_len, p->rx_ctrl.rssi);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(300);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  // !! Copiar esta MAC en masterMAC[] del test_sta_joystick !!
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  Serial.printf("Master STA MAC: %02X:%02X:%02X:%02X:%02X:%02X  canal: %d\n",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], WiFi.channel());

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
