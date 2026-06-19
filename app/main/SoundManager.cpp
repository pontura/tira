#include "SoundManager.h"
#include <NonBlockingRtttl.h>

void SoundManager::begin() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void SoundManager::play(const char* rtttl) {
  rtttl::begin(BUZZER_PIN, rtttl);
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
