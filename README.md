# Puddles

An Arduino Nano IoT 33 project that drives a two-axis eyeball mechanism using servos, controlled over MQTT.

## Hardware

- **Board:** Arduino Nano 33 IoT
- **Servo X (horizontal):** Tower SG-5010 on pin D20
- **Servo Y (vertical):** Tower GR92R on pin D21
- **Piezo buzzer:** Pin D12 (sound feedback via CuteBuzzerSounds)

## How It Works

Puddles connects to Wi-Fi and subscribes to two MQTT topics:

- **Tracking topic** — receives face-tracking coordinates (x, y, active) from a camera system and maps them to servo angles so the eyeball follows a detected face.
- **Control topic** — accepts manual commands from rotary encoders to position the servos directly, plus key commands for test sweeps.

When a face appears after an absence, Puddles plays a happy sound. When a face disappears for a while, it plays a sad sound.

Built with [PlatformIO](https://platformio.org/).

## Libraries

- Servo
- WiFiNINA
- PubSubClient (MQTT)
- ArduinoJson
- CuteBuzzerSounds (bundled in `lib/`)
- RobotSoundEngine (bundled in `lib/`)