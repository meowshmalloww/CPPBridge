#pragma once
// =============================================================================
// AUDIO.H - Simple Audio Playback Module
// =============================================================================
// Basic audio playback using Windows PlaySound API.
// =============================================================================

#include <windows.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#pragma comment(lib, "winmm.lib")

namespace Hub::Media {

// =============================================================================
// SIMPLE AUDIO PLAYER
// =============================================================================
class AudioPlayer {
public:
  ~AudioPlayer() { stop(); }

  // Play WAV file asynchronously
  bool playWav(const std::string &filepath, bool loop = false) {
    stop();
    filepath_ = filepath;
    playing_ = true;
    looping_ = loop;

    DWORD flags = SND_FILENAME | SND_ASYNC;
    if (loop)
      flags |= SND_LOOP;

    return PlaySoundA(filepath.c_str(), nullptr, flags) != 0;
  }

  // Play system sound
  bool playSystemSound(const char *sound) {
    return PlaySoundA(sound, nullptr, SND_ALIAS | SND_ASYNC) != 0;
  }

  // Play beep/tone
  static bool playTone(int frequency = 440, int duration_ms = 200) {
    return Beep(frequency, duration_ms) != 0;
  }

  // Stop
  void stop() {
    if (playing_) {
      PlaySoundA(nullptr, nullptr, 0);
      playing_ = false;
    }
  }

  bool isPlaying() const { return playing_; }

private:
  std::string filepath_;
  std::atomic<bool> playing_{false};
  bool looping_ = false;
};

// =============================================================================
// AUDIO PLAYER STORAGE
// =============================================================================
inline std::mutex audioMutex;
inline std::map<int, std::unique_ptr<AudioPlayer>> audioPlayers;
inline std::atomic<int> audioNextHandle{1};

inline int createAudioPlayer() {
  std::lock_guard<std::mutex> lock(audioMutex);
  int handle = audioNextHandle++;
  audioPlayers[handle] = std::make_unique<AudioPlayer>();
  return handle;
}

inline AudioPlayer *getAudioPlayer(int handle) {
  std::lock_guard<std::mutex> lock(audioMutex);
  auto it = audioPlayers.find(handle);
  return (it != audioPlayers.end()) ? it->second.get() : nullptr;
}

inline void releaseAudioPlayer(int handle) {
  std::lock_guard<std::mutex> lock(audioMutex);
  audioPlayers.erase(handle);
}

} // namespace Hub::Media

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_audio_create();
__declspec(dllexport) int hub_audio_play_wav(int handle, const char *filepath,
                                             int loop);
__declspec(dllexport) int hub_audio_play_tone(int frequency, int duration_ms);
__declspec(dllexport) void hub_audio_stop(int handle);
__declspec(dllexport) void hub_audio_release(int handle);
}
