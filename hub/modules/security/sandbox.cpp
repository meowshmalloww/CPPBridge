// =============================================================================
// SANDBOX.CPP - Sandboxing Implementation
// =============================================================================

#include "sandbox.h"
#include <algorithm>
#include <cstring>


static thread_local std::string tl_error;

extern "C" {

__declspec(dllexport) void hub_sandbox_add_path(const char *path) {
  if (!path)
    return;
  Hub::Security::PathValidator::instance().addAllowedPath(path);
}

__declspec(dllexport) void hub_sandbox_remove_path(const char *path) {
  if (!path)
    return;
  Hub::Security::PathValidator::instance().removeAllowedPath(path);
}

__declspec(dllexport) void hub_sandbox_clear() {
  Hub::Security::PathValidator::instance().clearAllowedPaths();
}

__declspec(dllexport) int hub_sandbox_is_safe(const char *path) {
  if (!path)
    return 0;
  return Hub::Security::PathValidator::instance().isPathSafe(path) ? 1 : 0;
}

__declspec(dllexport) int
hub_sandbox_validate(const char *path, char *error_buf, int error_size) {
  if (!path) {
    if (error_buf && error_size > 0) {
      strncpy(error_buf, "Null path", error_size - 1);
      error_buf[error_size - 1] = '\0';
    }
    return 0;
  }

  auto result = Hub::Security::PathValidator::instance().validate(path);

  if (!result.valid && error_buf && error_size > 0) {
    strncpy(error_buf, result.error.c_str(), error_size - 1);
    error_buf[error_size - 1] = '\0';
  }

  return result.valid ? 1 : 0;
}

__declspec(dllexport) int hub_input_validate_string(const char *str,
                                                    int max_len) {
  return Hub::Security::InputValidator::isValidString(str, max_len > 0 ? max_len
                                                                       : 65536)
             ? 1
             : 0;
}

} // extern "C"
