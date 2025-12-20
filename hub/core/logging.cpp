// =============================================================================
// LOGGING.CPP - Logging Implementation
// =============================================================================

#include "logging.h"

static thread_local std::string tl_current_file;

extern "C" {

__declspec(dllexport) void hub_log_init(const char *log_dir, const char *prefix,
                                        int max_size_mb, int max_files) {
  Hub::RotatingLogger::instance().init(
      log_dir ? log_dir : "logs", prefix ? prefix : "app",
      max_size_mb > 0 ? max_size_mb * 1024 * 1024 : 10 * 1024 * 1024,
      max_files > 0 ? max_files : 5);
}

__declspec(dllexport) void hub_log_set_level(int level) {
  if (level >= 0 && level <= 6) {
    Hub::RotatingLogger::instance().setLevel(static_cast<Hub::LogLevel>(level));
  }
}

__declspec(dllexport) void hub_log_set_console(int enabled) {
  Hub::RotatingLogger::instance().setConsoleOutput(enabled != 0);
}

__declspec(dllexport) void hub_log(int level, const char *message,
                                   const char *source) {
  if (level >= 0 && level <= 5 && message) {
    Hub::RotatingLogger::instance().log(static_cast<Hub::LogLevel>(level),
                                        message, source ? source : "");
  }
}

__declspec(dllexport) void hub_log_info(const char *message) {
  if (message)
    Hub::RotatingLogger::instance().info(message);
}

__declspec(dllexport) void hub_log_warn(const char *message) {
  if (message)
    Hub::RotatingLogger::instance().warn(message);
}

__declspec(dllexport) void hub_log_error(const char *message) {
  if (message)
    Hub::RotatingLogger::instance().error(message);
}

__declspec(dllexport) const char *hub_log_current_file() {
  tl_current_file = Hub::RotatingLogger::instance().currentLogFile();
  return tl_current_file.c_str();
}

__declspec(dllexport) void hub_log_flush() {
  Hub::RotatingLogger::instance().flush();
}

__declspec(dllexport) void hub_log_shutdown() {
  Hub::RotatingLogger::instance().shutdown();
}

} // extern "C"
