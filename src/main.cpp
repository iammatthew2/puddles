#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Servo.h>
#include <WiFiNINA.h>

#include "secrets.h"

Servo rudderServo;
constexpr uint8_t SERVO_PIN = 21;  // D21 on Nano 33 IoT
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 10000;
constexpr uint32_t MQTT_RETRY_INTERVAL_MS = 5000;
constexpr int CAMERA_X_MIN = 0;
constexpr int CAMERA_X_MAX = 640;

uint32_t lastWiFiAttemptMs = 0;
uint32_t lastMqttAttemptMs = 0;

int mapTrackingXToServo(int x) {
  int constrainedX = constrain(x, CAMERA_X_MIN, CAMERA_X_MAX);
  return map(constrainedX, CAMERA_X_MIN, CAMERA_X_MAX, 0, 180);
}

void onMqttMessage(char* topic, uint8_t* payload, unsigned int length) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("MQTT JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }

  bool active = doc["active"] | false;
  int x = doc["x"] | (CAMERA_X_MAX / 2);
  int y = doc["y"] | -1;
  int dist = doc["dist"] | -1;

  Serial.print("Tracking topic: ");
  Serial.println(topic);
  Serial.print("x=");
  Serial.print(x);
  Serial.print(" y=");
  Serial.print(y);
  Serial.print(" dist=");
  Serial.print(dist);
  Serial.print(" active=");
  Serial.println(active ? "true" : "false");

  if (!active) {
    return;
  }

  int servoAngle = mapTrackingXToServo(x);
  rudderServo.write(servoAngle);

  Serial.print("Servo angle set to ");
  Serial.println(servoAngle);
}

void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  uint32_t now = millis();
  if (now - lastWiFiAttemptMs < WIFI_RETRY_INTERVAL_MS) {
    return;
  }
  lastWiFiAttemptMs = now;

  Serial.print("Connecting to Wi-Fi SSID: ");
  Serial.println(WIFI_SSID);

  const uint32_t startMs = millis();
  WiFi.disconnect();
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED &&
         (millis() - startMs) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print('.');
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
    return;
  }

  Serial.print("Wi-Fi failed, status=");
  Serial.println(WiFi.status());
}

void connectToMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) {
    return;
  }

  uint32_t now = millis();
  if (now - lastMqttAttemptMs < MQTT_RETRY_INTERVAL_MS) {
    return;
  }
  lastMqttAttemptMs = now;

  Serial.print("Connecting to MQTT broker: ");
  Serial.print(MQTT_BROKER);
  Serial.print(":");
  Serial.println(MQTT_PORT);

  bool connected = mqttClient.connect(MQTT_CLIENT_ID);

  if (connected) {
    Serial.println("MQTT connected");

    if (mqttClient.subscribe(MQTT_TRACKING_TOPIC)) {
      Serial.print("Subscribed to ");
      Serial.println(MQTT_TRACKING_TOPIC);
    } else {
      Serial.print("Failed to subscribe to ");
      Serial.println(MQTT_TRACKING_TOPIC);
    }

    return;
  }

  Serial.print("MQTT connect failed, rc=");
  Serial.println(mqttClient.state());
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  rudderServo.attach(SERVO_PIN);
  rudderServo.write(90);

  Serial.println("Servo attached on D21, set to 90 degrees");
  connectToWiFi();

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  connectToMqtt();
}

void loop() {
  connectToWiFi();
  connectToMqtt();

  mqttClient.loop();
}