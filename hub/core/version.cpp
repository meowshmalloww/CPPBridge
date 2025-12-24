// =============================================================================
// VERSION.CPP - Version Compatibility C API
// =============================================================================

#include "version.h"

static thread_local std::string tl_version;
static thread_local std::string tl_version_json;

extern "C" {

__declspec(dllexport) const char *hub_version_get() {
  tl_version = Hub::Core::get_version();
  return tl_version.c_str();
}

__declspec(dllexport) int hub_version_major() {
  return Hub::Core::get_version_major();
}

__declspec(dllexport) int hub_version_minor() {
  return Hub::Core::get_version_minor();
}

__declspec(dllexport) int hub_version_patch() {
  return Hub::Core::get_version_patch();
}

__declspec(dllexport) int hub_version_number() {
  return Hub::Core::get_version_number();
}

__declspec(dllexport) int hub_version_check_compat(const char *min_version) {
  if (!min_version)
    return 1; // If no min specified, always compatible
  return Hub::Core::check_compat(min_version) ? 1 : 0;
}

__declspec(dllexport) int hub_version_check_range(const char *min_version,
                                                  const char *max_version) {
  if (!min_version || !max_version)
    return 1;
  return Hub::Core::check_compat_range(min_version, max_version) ? 1 : 0;
}

__declspec(dllexport) int hub_version_allow_any() {
  return 1; // Always returns true - allows any version
}

__declspec(dllexport) const char *hub_version_info() {
  tl_version_json = Hub::Core::get_version_info_json();
  return tl_version_json.c_str();
}

} // extern "C"
