#pragma once
// =============================================================================
// LOGGER.H - Thread-Safe Logging System
// =============================================================================
// High-performance logging with file output and console support.
// NOTE: Uses LEVEL_ prefix to avoid Windows macro conflicts with ERROR
// =============================================================================

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>


namespace Hub {

// =============================================================================
// LOG LEVELS (prefixed to avoid Windows macro conflicts)
// =============================================================================
enum class LogLevel {
  LEVEL_DEBUG = 0,
  LEVEL_INFO = 1,
  LEVEL_WARN = 2,
  LEVEL_ERROR = 3,
  LEVEL_FATAL = 4
};

inline const char *log_level_to_string(LogLevel level) {
  switch (level) {
  case LogLevel::LEVEL_DEBUG:
    return "DEBUG";
  case LogLevel::LEVEL_INFO:
    return "INFO ";
  case LogLevel::LEVEL_WARN:
    return "WARN ";
  case LogLevel::LEVEL_ERROR:
    return "ERROR";
  case LogLevel::LEVEL_FATAL:
    return "FATAL";
  default:
    return "?????";
  }
}

// =============================================================================
// LOGGER CLASS
// =============================================================================
class Logger {
public:
  static Logger &instance() {
    static Logger inst;
    return inst;
  }

  void set_file(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open())
      file_.close();
    file_.open(path, std::ios::app);
    file_path_ = path;
  }

  void set_min_level(LogLevel level) { min_level_ = level; }

  void set_console_output(bool enabled) { console_enabled_ = enabled; }

  void log(LogLevel level, const std::string &context,
           const std::string &message) {
    if (level < min_level_)
      return;

    std::lock_guard<std::mutex> lock(mutex_);

    // Format timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t_val = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::ostringstream oss;
    struct tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &time_t_val);
#else
    localtime_r(&time_t_val, &timeinfo);
#endif
    oss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    // Build log line
    std::string line = "[" + oss.str() + "] [" +
                       std::string(log_level_to_string(level)) + "] [" +
                       context + "] " + message + "\n";

    // Write to file
    if (file_.is_open()) {
      file_ << line;
      file_.flush();
    }

    // Write to console
    if (console_enabled_) {
      if (level >= LogLevel::LEVEL_ERROR) {
        std::cerr << line;
      } else {
        std::cout << line;
      }
    }
  }

  void debug(const std::string &context, const std::string &message) {
    log(LogLevel::LEVEL_DEBUG, context, message);
  }

  void info(const std::string &context, const std::string &message) {
    log(LogLevel::LEVEL_INFO, context, message);
  }

  void warn(const std::string &context, const std::string &message) {
    log(LogLevel::LEVEL_WARN, context, message);
  }

  void error(const std::string &context, const std::string &message) {
    log(LogLevel::LEVEL_ERROR, context, message);
  }

  void fatal(const std::string &context, const std::string &message) {
    log(LogLevel::LEVEL_FATAL, context, message);
  }

private:
  Logger() : min_level_(LogLevel::LEVEL_INFO), console_enabled_(true) {}
  ~Logger() {
    if (file_.is_open())
      file_.close();
  }

  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  std::mutex mutex_;
  std::ofstream file_;
  std::string file_path_;
  LogLevel min_level_;
  bool console_enabled_;
};

// =============================================================================
// CONVENIENCE MACROS
// =============================================================================
#define LOG_DEBUG(ctx, msg) Hub::Logger::instance().debug(ctx, msg)
#define LOG_INFO(ctx, msg) Hub::Logger::instance().info(ctx, msg)
#define LOG_WARN(ctx, msg) Hub::Logger::instance().warn(ctx, msg)
#define LOG_ERROR(ctx, msg) Hub::Logger::instance().error(ctx, msg)
#define LOG_FATAL(ctx, msg) Hub::Logger::instance().fatal(ctx, msg)

} // namespace Hub
