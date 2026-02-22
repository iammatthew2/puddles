#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Servo.h>
#include <WiFiNINA.h>

#include "RobotSoundEngine.h"
#include "secrets.h"

Servo servoX;  // D20 - X-axis (tower sg-5010)
Servo servoY;  // D21 - Y-axis (tower gr92r)
constexpr uint8_t SERVO_X_PIN = 20;
constexpr uint8_t SERVO_Y_PIN = 21;
constexpr uint8_t PIEZO_BUZZER_PIN = 12;
RobotSoundEngine soundEngine(VOICE_NEUTRAL, PIEZO_BUZZER_PIN);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 10000;
constexpr uint32_t MQTT_RETRY_INTERVAL_MS = 5000;
constexpr int CAMERA_X_MIN = 0;
constexpr int CAMERA_X_MAX = 640;
constexpr int CAMERA_Y_MIN = 0;
constexpr int CAMERA_Y_MAX = 480;
constexpr int SERVO_X_MIN_ANGLE = 40;
constexpr int SERVO_X_MAX_ANGLE = 140;
constexpr int SERVO_Y_MIN_ANGLE = 50;
constexpr int SERVO_Y_MAX_ANGLE = 130;
constexpr int SERVO_JITTER_DEADBAND_DEGREES = 2;
constexpr uint32_t FACE_STATE_SOUND_DELAY_MS = 10000;

uint32_t lastWiFiAttemptMs = 0;
uint32_t lastMqttAttemptMs = 0;
int lastServoXAngle = 90;
int lastServoYAngle = 90;
uint32_t lastFaceActiveMs = 0;
bool faceActive = false;
bool hasSeenFace = false;
bool sadQueuedForCurrentAbsence = false;
bool happySoundPending = false;
bool sadSoundPending = false;

int mapTrackingXToServo(int x) {
  int constrainedX = constrain(x, CAMERA_X_MIN, CAMERA_X_MAX);
  return map(constrainedX, CAMERA_X_MIN, CAMERA_X_MAX, SERVO_X_MIN_ANGLE,
             SERVO_X_MAX_ANGLE);
}

int mapTrackingYToServo(int y) {
  int constrainedY = constrain(y, CAMERA_Y_MIN, CAMERA_Y_MAX);
  return map(constrainedY, CAMERA_Y_MIN, CAMERA_Y_MAX, SERVO_Y_MIN_ANGLE,
             SERVO_Y_MAX_ANGLE);
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

  uint32_t now = millis();

  if (active) {
    bool wasAbsentLongEnough = (!faceActive) && ((now - lastFaceActiveMs) >=
                                                 FACE_STATE_SOUND_DELAY_MS);

    if (wasAbsentLongEnough) {
      happySoundPending = true;
    }

    faceActive = true;
    hasSeenFace = true;
    sadQueuedForCurrentAbsence = false;
    lastFaceActiveMs = now;
  } else {
    faceActive = false;
  }

  if (!active) {
    return;
  }

  int servoXAngle = mapTrackingXToServo(x);
  int servoYAngle = mapTrackingYToServo(y);

  if (abs(servoXAngle - lastServoXAngle) > SERVO_JITTER_DEADBAND_DEGREES) {
    servoX.write(servoXAngle);
    lastServoXAngle = servoXAngle;
  }

  if (abs(servoYAngle - lastServoYAngle) > SERVO_JITTER_DEADBAND_DEGREES) {
    servoY.write(servoYAngle);
    lastServoYAngle = servoYAngle;
  }

  Serial.print("Servo X angle: ");
  Serial.print(servoXAngle);
  Serial.print(" | Servo Y angle: ");
  Serial.println(servoYAngle);
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

  esp_err_t soundInitResult = soundEngine.init();
  if (soundInitResult == ESP_OK) {
    Serial.println("RobotSoundEngine initialized on D12");
  } else {
    Serial.println("RobotSoundEngine init failed");
  }

  servoX.attach(SERVO_X_PIN);
  servoX.write(lastServoXAngle);
  Serial.println("Servo X attached on D20, set to 90 degrees");

  servoY.attach(SERVO_Y_PIN);
  servoY.write(lastServoYAngle);
  Serial.println("Servo Y attached on D21, set to 90 degrees");
  connectToWiFi();

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  connectToMqtt();
}

void loop() {
  connectToWiFi();
  connectToMqtt();

  mqttClient.loop();

  if (!faceActive && hasSeenFace && !sadQueuedForCurrentAbsence &&
      ((millis() - lastFaceActiveMs) >= FACE_STATE_SOUND_DELAY_MS)) {
    sadSoundPending = true;
    sadQueuedForCurrentAbsence = true;
  }

  if (happySoundPending) {
    happySoundPending = false;
    soundEngine.playEmotion(EMOTION_HAPPY, 0.5f);
  }

  if (sadSoundPending) {
    sadSoundPending = false;
    soundEngine.playEmotion(EMOTION_SAD, 0.5f);
  }
}