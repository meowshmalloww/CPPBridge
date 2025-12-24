#pragma once
// =============================================================================
// POWER.H - Power Management Module
// =============================================================================
// Prevent sleep, get battery info, power state management.
// Uses Windows SetThreadExecutionState and GetSystemPowerStatus.
// =============================================================================

#ifdef _WIN32
#include <windows.h>
#endif

#include <string>

namespace Hub::System {

// =============================================================================
// POWER STATE MANAGEMENT
// =============================================================================

// Prevent system from sleeping (call before long operations)
inline bool prevent_sleep() {
#ifdef _WIN32
  EXECUTION_STATE result = SetThreadExecutionState(
      ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
  return result != 0;
#else
  return false;
#endif
}

// Allow system to sleep again (call when done)
inline bool allow_sleep() {
#ifdef _WIN32
  EXECUTION_STATE result = SetThreadExecutionState(ES_CONTINUOUS);
  return result != 0;
#else
  return false;
#endif
}

// Prevent only display from sleeping (screen stays on)
inline bool keep_display_on() {
#ifdef _WIN32
  EXECUTION_STATE result =
      SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);
  return result != 0;
#else
  return false;
#endif
}

// =============================================================================
// BATTERY INFORMATION
// =============================================================================

struct BatteryInfo {
  int percent;           // 0-100, or -1 if unknown
  bool is_charging;      // true if plugged in
  bool has_battery;      // true if system has battery
  int seconds_remaining; // -1 if unknown
};

inline BatteryInfo get_battery_info() {
  BatteryInfo info = {-1, false, false, -1};

#ifdef _WIN32
  SYSTEM_POWER_STATUS status;
  if (GetSystemPowerStatus(&status)) {
    info.has_battery = (status.BatteryFlag != 128); // 128 = no battery
    info.is_charging = (status.ACLineStatus == 1);

    if (status.BatteryLifePercent != 255) {
      info.percent = status.BatteryLifePercent;
    }

    if (status.BatteryLifeTime != (DWORD)-1) {
      info.seconds_remaining = status.BatteryLifeTime;
    }
  }
#endif

  return info;
}

// Simple getters
inline int get_battery_percent() { return get_battery_info().percent; }

inline bool is_charging() { return get_battery_info().is_charging; }

inline bool has_battery() { return get_battery_info().has_battery; }

inline int get_battery_time_remaining() {
  return get_battery_info().seconds_remaining;
}

} // namespace Hub::System
