#include "SoundManager.h"
#include <NonBlockingRtttl.h>

void (*SoundManager::onPlayCallback)(const char*)            = nullptr;
void (*SoundManager::onJoystickCallback)(const char*)        = nullptr;
void (*SoundManager::onPlayerCallback)(uint8_t, const char*) = nullptr;

void SoundManager::begin() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void SoundManager::setOnPlayCallback(void (*cb)(const char*)) {
  onPlayCallback = cb;
}

void SoundManager::setOnJoystickCallback(void (*cb)(const char*)) {
  onJoystickCallback = cb;
}

void SoundManager::sendToJoysticks(const char* rtttl) {
  if (onJoystickCallback) onJoystickCallback(rtttl);
}

void SoundManager::setOnPlayerCallback(void (*cb)(uint8_t, const char*)) {
  onPlayerCallback = cb;
}

void SoundManager::sendToPlayer(uint8_t player, const char* rtttl) {
  if (onPlayerCallback) onPlayerCallback(player, rtttl);
}

void SoundManager::play(const char* rtttl) {
  rtttl::begin(BUZZER_PIN, rtttl);
  if (onPlayCallback) onPlayCallback(rtttl);
}

void SoundManager::update() {
  rtttl::play();
}

void SoundManager::stop() {
  rtttl::stop();
  noTone(BUZZER_PIN);
}

bool SoundManager::isPlaying() {
  return rtttl::isPlaying();
}
