#include <Arduino.h>
#include <Servo.h>
#include <WiFiNINA.h>

#include "secrets.h"

Servo rudderServo;
constexpr uint8_t SERVO_PIN = 21;  // D21 on Nano 33 IoT

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;

void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi SSID: ");
  Serial.println(WIFI_SSID);

  const uint32_t startMs = millis();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED &&
         (millis() - startMs) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(500);
    Serial.print('.');
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return;
  }

  Serial.println("Wi-Fi connection failed (timeout)");
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  rudderServo.attach(SERVO_PIN);
  rudderServo.write(90);

  Serial.println("Servo attached on D21, set to 90 degrees");
  connectToWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  delay(1000);
}