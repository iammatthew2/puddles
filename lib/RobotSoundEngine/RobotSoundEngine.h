#ifndef ROBOT_SOUND_ENGINE_H
#define ROBOT_SOUND_ENGINE_H

#include <Arduino.h>

#ifndef ESP_OK
typedef int esp_err_t;
constexpr esp_err_t ESP_OK = 0;
constexpr esp_err_t ESP_ERR_INVALID_ARG = -1;
#endif

enum VoiceProfile {
  VOICE_GRUFF,
  VOICE_NEUTRAL,
  VOICE_CHIPPER,
  VOICE_TINY,
  VOICE_DEEP
};

enum Emotion {
  EMOTION_HAPPY,
  EMOTION_SAD,
  EMOTION_CURIOUS,
  EMOTION_SHOCK,
  EMOTION_SORROW,
  EMOTION_EXCITED,
  EMOTION_CONFUSED,
  EMOTION_DELIGHTED,
  EMOTION_SATISFIED,
  EMOTION_TRIUMPHANT
};

struct VoiceConfig {
  float freq_multiplier;
  float tempo_scale;
  float pitch_variation;
};

class RobotSoundEngine {
 public:
  RobotSoundEngine(VoiceProfile voice = VOICE_NEUTRAL, int buzzer_pin = -1,
                   int led_pin = -1);

  esp_err_t init();
  void playTone(float frequency_hz, uint32_t duration_ms = 0,
                bool blocking = false);
  void stop();
  bool isPlaying();
  void setVolume(float volume);
  void playEmotion(Emotion emotion, float intensity = 0.7f);
  void setVoice(VoiceProfile voice);

  bool hasLight() const;
  void stopLight();

 private:
  void pulse(float frequency_hz, uint32_t duration_ms, bool blocking);
  VoiceConfig getVoiceConfig(VoiceProfile voice) const;
  float applyVoice(float base_frequency) const;
  uint32_t applyVoiceTiming(uint32_t base_duration) const;
  void lightOn();

  int buzzer_pin_;
  int led_pin_;
  bool initialized_;
  bool light_initialized_;
  bool playing_;
  uint32_t playing_until_ms_;
  float current_volume_;
  VoiceProfile current_voice_;
  VoiceConfig voice_config_;
};

#endif
