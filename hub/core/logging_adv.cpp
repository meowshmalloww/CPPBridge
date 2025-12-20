// =============================================================================
// LOGGING_ADV.CPP - Advanced Async Logging Implementation
// =============================================================================

#include "logging_adv.h"
#include <cstring>

static std::string g_currentFile;

// Simple JSON field parser for FFI
static std::map<std::string, std::string> parseJsonFields(const char *json) {
  std::map<std::string, std::string> result;
  if (!json || json[0] != '{')
    return result;

  std::string s(json);
  size_t pos = 1;

  while (pos < s.size()) {
    // Find key
    size_t keyStart = s.find('"', pos);
    if (keyStart == std::string::npos)
      break;
    size_t keyEnd = s.find('"', keyStart + 1);
    if (keyEnd == std::string::npos)
      break;

    std::string key = s.substr(keyStart + 1, keyEnd - keyStart - 1);

    // Find value
    size_t colonPos = s.find(':', keyEnd);
    if (colonPos == std::string::npos)
      break;

    size_t valStart = s.find('"', colonPos);
    if (valStart == std::string::npos)
      break;
    size_t valEnd = s.find('"', valStart + 1);
    if (valEnd == std::string::npos)
      break;

    std::string val = s.substr(valStart + 1, valEnd - valStart - 1);

    result[key] = val;
    pos = valEnd + 1;
  }

  return result;
}

extern "C" {

__declspec(dllexport) void hub_log_adv_init(const char *log_dir,
                                            const char *prefix, int max_size_mb,
                                            int max_files) {
  if (!log_dir)
    return;

  try {
    Hub::AsyncLogger::instance().init(log_dir, prefix ? prefix : "app",
                                      static_cast<size_t>(max_size_mb) * 1024 *
                                          1024,
                                      max_files > 0 ? max_files : 5);
  } catch (...) {
  }
}

__declspec(dllexport) void hub_log_adv_set_level(int level) {
  try {
    Hub::AsyncLogger::instance().setLevel(static_cast<Hub::LogLvl>(level));
  } catch (...) {
  }
}

__declspec(dllexport) void hub_log_adv_set_json(int enabled) {
  try {
    Hub::AsyncLogger::instance().setFormat(
        enabled ? Hub::AsyncLogger::Format::JSON
                : Hub::AsyncLogger::Format::TEXT);
  } catch (...) {
  }
}

__declspec(dllexport) void hub_log_adv_set_console(int enabled) {
  try {
    Hub::AsyncLogger::instance().setConsole(enabled != 0);
  } catch (...) {
  }
}

__declspec(dllexport) void hub_log_adv(int level, const char *message,
                                       const char *source) {
  if (!message)
    return;

  try {
    Hub::AsyncLogger::instance().log(static_cast<Hub::LogLvl>(level), message,
                                     source ? source : "");
  } catch (...) {
  }
}

__declspec(dllexport) void hub_log_adv_structured(int level,
                                                  const char *message,
                                                  const char *json_fields) {
  if (!message)
    return;

  try {
    auto fields = parseJsonFields(json_fields);
    Hub::AsyncLogger::instance().logStructured(static_cast<Hub::LogLvl>(level),
                                               message, fields);
  } catch (...) {
  }
}

__declspec(dllexport) const char *hub_log_adv_current_file() {
  try {
    g_currentFile = Hub::AsyncLogger::instance().currentFile();
    return g_currentFile.c_str();
  } catch (...) {
    return "";
  }
}

__declspec(dllexport) void hub_log_adv_flush() {
  try {
    Hub::AsyncLogger::instance().flush();
  } catch (...) {
  }
}

__declspec(dllexport) void hub_log_adv_shutdown() {
  try {
    Hub::AsyncLogger::instance().shutdown();
  } catch (...) {
  }
}

} // extern "C"
