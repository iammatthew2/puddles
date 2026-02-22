#include "RobotSoundEngine.h"

RobotSoundEngine::RobotSoundEngine(VoiceProfile voice, int buzzer_pin,
                                   int led_pin)
    : buzzer_pin_(buzzer_pin),
      led_pin_(led_pin),
      initialized_(false),
      light_initialized_(false),
      playing_(false),
      playing_until_ms_(0),
      current_volume_(0.5f),
      current_voice_(voice),
      voice_config_(getVoiceConfig(voice)) {}

esp_err_t RobotSoundEngine::init() {
  if (buzzer_pin_ < 0) {
    return ESP_ERR_INVALID_ARG;
  }

  pinMode(buzzer_pin_, OUTPUT);
  noTone(buzzer_pin_);

  if (led_pin_ >= 0) {
    pinMode(led_pin_, OUTPUT);
    digitalWrite(led_pin_, LOW);
    light_initialized_ = true;
  }

  initialized_ = true;
  return ESP_OK;
}

void RobotSoundEngine::lightOn() {
  if (light_initialized_) {
    digitalWrite(led_pin_, HIGH);
  }
}

void RobotSoundEngine::pulse(float frequency_hz, uint32_t duration_ms,
                             bool blocking) {
  if (!initialized_) {
    return;
  }

  if (frequency_hz < 20.0f) frequency_hz = 20.0f;
  if (frequency_hz > 10000.0f) frequency_hz = 10000.0f;

  lightOn();

  if (duration_ms == 0) {
    tone(buzzer_pin_, (unsigned int)frequency_hz);
    playing_ = true;
    playing_until_ms_ = 0;
    return;
  }

  tone(buzzer_pin_, (unsigned int)frequency_hz, duration_ms);
  playing_ = true;
  playing_until_ms_ = millis() + duration_ms;

  if (blocking) {
    delay(duration_ms);
    stop();
  }
}

void RobotSoundEngine::playTone(float frequency_hz, uint32_t duration_ms,
                                bool blocking) {
  pulse(applyVoice(frequency_hz), applyVoiceTiming(duration_ms), blocking);
}

void RobotSoundEngine::stop() {
  if (!initialized_) {
    return;
  }

  noTone(buzzer_pin_);
  playing_ = false;
  playing_until_ms_ = 0;
  stopLight();
}

bool RobotSoundEngine::isPlaying() {
  if (!playing_) {
    return false;
  }

  if (playing_until_ms_ > 0 && millis() >= playing_until_ms_) {
    playing_ = false;
    stopLight();
  }

  return playing_;
}

void RobotSoundEngine::setVolume(float volume) {
  if (volume < 0.0f) volume = 0.0f;
  if (volume > 1.0f) volume = 1.0f;
  current_volume_ = volume;
}

VoiceConfig RobotSoundEngine::getVoiceConfig(VoiceProfile voice) const {
  switch (voice) {
    case VOICE_GRUFF:
      return {0.7f, 0.8f, 0.0f};
    case VOICE_CHIPPER:
      return {1.6f, 1.3f, 0.0f};
    case VOICE_TINY:
      return {2.0f, 1.5f, 0.0f};
    case VOICE_DEEP:
      return {0.5f, 0.7f, 0.0f};
    case VOICE_NEUTRAL:
    default:
      return {1.0f, 1.0f, 0.0f};
  }
}

float RobotSoundEngine::applyVoice(float base_frequency) const {
  float scaled = base_frequency * voice_config_.freq_multiplier;
  if (scaled < 20.0f) scaled = 20.0f;
  if (scaled > 10000.0f) scaled = 10000.0f;
  return scaled;
}

uint32_t RobotSoundEngine::applyVoiceTiming(uint32_t base_duration) const {
  if (base_duration == 0) {
    return 0;
  }
  float scaled = (float)base_duration / voice_config_.tempo_scale;
  return (uint32_t)scaled;
}

void RobotSoundEngine::setVoice(VoiceProfile voice) {
  current_voice_ = voice;
  voice_config_ = getVoiceConfig(voice);
}

void RobotSoundEngine::playEmotion(Emotion emotion, float intensity) {
  if (intensity < 0.0f) intensity = 0.0f;
  if (intensity > 1.0f) intensity = 1.0f;

  uint16_t base = (uint16_t)(70 + intensity * 120);

  switch (emotion) {
    case EMOTION_HAPPY:
    case EMOTION_DELIGHTED:
    case EMOTION_TRIUMPHANT:
      playTone(900, base, true);
      delay(30);
      playTone(1200, base, true);
      break;
    case EMOTION_SAD:
    case EMOTION_SORROW:
      playTone(550, base + 60, true);
      delay(25);
      playTone(420, base + 80, true);
      break;
    case EMOTION_SHOCK:
      playTone(1400, 120, true);
      break;
    case EMOTION_CURIOUS:
    case EMOTION_CONFUSED:
      playTone(700, 80, true);
      delay(20);
      playTone(820, 90, true);
      break;
    case EMOTION_EXCITED:
      playTone(1000, 70, true);
      delay(20);
      playTone(1300, 80, true);
      delay(20);
      playTone(1600, 90, true);
      break;
    case EMOTION_SATISFIED:
    default:
      playTone(800, 120, true);
      break;
  }
}

bool RobotSoundEngine::hasLight() const { return light_initialized_; }

void RobotSoundEngine::stopLight() {
  if (light_initialized_) {
    digitalWrite(led_pin_, LOW);
  }
}
