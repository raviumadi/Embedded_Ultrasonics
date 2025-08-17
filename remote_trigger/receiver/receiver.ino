#include <WiFi.h>
#include <esp_now.h>

#define TRIGGER_PIN_TAP 6
#define TRIGGER_PIN_HOLD 4

typedef struct struct_message {
  uint8_t trigger;
} struct_message;

volatile bool tapNow = false;
volatile bool holdNow = false;
unsigned long tapStart = 0;
unsigned long holdStart = 0;
unsigned long lastValidTapTrigger = 0;
unsigned long lastValidHoldTrigger = 0;

void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
  struct_message msg;
  memcpy(&msg, incomingData, sizeof(msg));
  unsigned long now = millis();

  if (msg.trigger == 1 && !tapNow && now - lastValidTapTrigger > 200) {
    tapNow = true;
    tapStart = now;
    lastValidTapTrigger = now;
    digitalWrite(TRIGGER_PIN_TAP, LOW);
    Serial.print("TAP trigger received: GPIO LOW at ms: ");
    Serial.println(tapStart);
  }

  else if (msg.trigger == 2 && !holdNow && now - lastValidHoldTrigger > 200) {
    holdNow = true;
    holdStart = now;
    lastValidHoldTrigger = now;
    digitalWrite(TRIGGER_PIN_HOLD, LOW);
    Serial.print("HOLD trigger received: GPIO LOW at ms: ");
    Serial.println(holdStart);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIGGER_PIN_TAP, OUTPUT);
  pinMode(TRIGGER_PIN_HOLD, OUTPUT);
  digitalWrite(TRIGGER_PIN_TAP, HIGH);
  digitalWrite(TRIGGER_PIN_HOLD, HIGH);

  WiFi.mode(WIFI_STA);
  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (1);
  }

  esp_now_register_recv_cb(onReceive);
  Serial.println("ESP-NOW receiver ready.");
}

void loop() {
  unsigned long now = millis();

  if (tapNow && now - tapStart > 50) {
    digitalWrite(TRIGGER_PIN_TAP, HIGH);
    tapNow = false;
    Serial.println("TAP GPIO HIGH");
  }

  if (holdNow && now - holdStart > 50) {
    digitalWrite(TRIGGER_PIN_HOLD, HIGH);
    holdNow = false;
    Serial.println("HOLD GPIO HIGH");
  }
}