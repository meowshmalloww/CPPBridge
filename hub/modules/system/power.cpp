// =============================================================================
// POWER.CPP - Power Management C API
// =============================================================================

#include "power.h"

extern "C" {

__declspec(dllexport) int hub_power_prevent_sleep() {
  return Hub::System::prevent_sleep() ? 1 : 0;
}

__declspec(dllexport) int hub_power_allow_sleep() {
  return Hub::System::allow_sleep() ? 1 : 0;
}

__declspec(dllexport) int hub_power_keep_display_on() {
  return Hub::System::keep_display_on() ? 1 : 0;
}

__declspec(dllexport) int hub_power_get_battery_percent() {
  return Hub::System::get_battery_percent();
}

__declspec(dllexport) int hub_power_is_charging() {
  return Hub::System::is_charging() ? 1 : 0;
}

__declspec(dllexport) int hub_power_has_battery() {
  return Hub::System::has_battery() ? 1 : 0;
}

__declspec(dllexport) int hub_power_get_time_remaining() {
  return Hub::System::get_battery_time_remaining();
}

} // extern "C"
