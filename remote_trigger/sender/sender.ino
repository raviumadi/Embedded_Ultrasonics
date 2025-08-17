#include <WiFi.h>
#include <esp_now.h>

#define BUTTON_TAP 4
#define BUTTON_HOLD 16

uint8_t receiverMac[] = { 0x34, 0xB7, 0xDA, 0x53, 0x78, 0xD4 }; // Receiver MAC

typedef struct struct_message {
  uint8_t trigger;
} struct_message;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_TAP, INPUT_PULLUP);
  pinMode(BUTTON_HOLD, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  Serial.print("Sender MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (1);
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    while (1);
  }

  Serial.println("ESP-NOW sender ready.");
}

void loop() {
  static bool lastTap = HIGH;
  static bool lastHold = HIGH;
  bool tapNow = digitalRead(BUTTON_TAP);
  bool holdNow = digitalRead(BUTTON_HOLD);

  struct_message msg;

  if (lastTap == HIGH && tapNow == LOW) {
    msg.trigger = 1;
    esp_now_send(receiverMac, (uint8_t *)&msg, sizeof(msg));
    Serial.println("Tap trigger sent");
    delay(50);
  }

  if (lastHold == HIGH && holdNow == LOW) {
    msg.trigger = 2;
    esp_now_send(receiverMac, (uint8_t *)&msg, sizeof(msg));
    Serial.println("Hold trigger sent");
    delay(50);
  }

  lastTap = tapNow;
  lastHold = holdNow;
}