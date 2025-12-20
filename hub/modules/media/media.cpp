// =============================================================================
// MEDIA.CPP - Media Module Implementation
// =============================================================================

#include "audio.h"
#include "image.h"

extern "C" {

// =============================================================================
// AUDIO FUNCTIONS
// =============================================================================
__declspec(dllexport) int hub_audio_create() {
  return Hub::Media::createAudioPlayer();
}

__declspec(dllexport) int hub_audio_play_wav(int handle, const char *filepath,
                                             int loop) {
  if (!filepath)
    return -1;
  auto *player = Hub::Media::getAudioPlayer(handle);
  if (!player)
    return -1;
  return player->playWav(filepath, loop != 0) ? 0 : -1;
}

__declspec(dllexport) int hub_audio_play_tone(int frequency, int duration_ms) {
  return Hub::Media::AudioPlayer::playTone(frequency, duration_ms) ? 0 : -1;
}

__declspec(dllexport) void hub_audio_stop(int handle) {
  auto *player = Hub::Media::getAudioPlayer(handle);
  if (player)
    player->stop();
}

__declspec(dllexport) void hub_audio_release(int handle) {
  Hub::Media::releaseAudioPlayer(handle);
}

// =============================================================================
// IMAGE/SCREEN FUNCTIONS
// =============================================================================
__declspec(dllexport) int hub_screen_width() {
  return GetSystemMetrics(SM_CXSCREEN);
}

__declspec(dllexport) int hub_screen_height() {
  return GetSystemMetrics(SM_CYSCREEN);
}

__declspec(dllexport) int hub_screen_capture(const char *filepath) {
  if (!filepath)
    return -1;

  auto img = Hub::Media::captureScreen();
  if (!img.isValid())
    return -1;

  return Hub::Media::saveBMP(img, filepath) ? 0 : -1;
}

} // extern "C"
