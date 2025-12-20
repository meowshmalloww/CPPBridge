#pragma once
// =============================================================================
// LOGGING.H - Enterprise Rotating Log System
// =============================================================================
// Features:
// - Rotating log files with size limits
// - Severity levels (DEBUG, INFO, WARN, ERROR, FATAL)
// - Timestamps and source tracking
// - Thread-safe async logging
// - Automatic cleanup of old logs
// =============================================================================

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

namespace Hub {

namespace fs = std::filesystem;

// =============================================================================
// LOG LEVELS
// =============================================================================
enum class LogLevel : int {
  TRACE = 0,
  DEBUG = 1,
  INFO = 2,
  WARN = 3,
  ERROR = 4,
  FATAL = 5,
  OFF = 6
};

inline const char *logLevelStr(LogLevel level) {
  switch (level) {
  case LogLevel::TRACE:
    return "TRACE";
  case LogLevel::DEBUG:
    return "DEBUG";
  case LogLevel::INFO:
    return "INFO ";
  case LogLevel::WARN:
    return "WARN ";
  case LogLevel::ERROR:
    return "ERROR";
  case LogLevel::FATAL:
    return "FATAL";
  default:
    return "?????";
  }
}

// =============================================================================
// LOG ENTRY
// =============================================================================
struct LogEntry {
  LogLevel level;
  std::string message;
  std::string source;
  std::chrono::system_clock::time_point timestamp;
  std::thread::id threadId;

  LogEntry() = default;
  LogEntry(LogLevel l, const std::string &msg, const std::string &src = "")
      : level(l), message(msg), source(src),
        timestamp(std::chrono::system_clock::now()),
        threadId(std::this_thread::get_id()) {}
};

// =============================================================================
// ROTATING FILE LOGGER
// =============================================================================
class RotatingLogger {
public:
  static RotatingLogger &instance() {
    static RotatingLogger inst;
    return inst;
  }

  // Initialize logger
  void init(const std::string &logDir, const std::string &prefix = "app",
            size_t maxFileSize = 10 * 1024 * 1024, // 10MB
            int maxFiles = 5) {
    std::lock_guard<std::mutex> lock(mutex_);

    logDir_ = logDir;
    prefix_ = prefix;
    maxFileSize_ = maxFileSize;
    maxFiles_ = maxFiles;
    minLevel_ = LogLevel::INFO;

    // Create log directory
    fs::create_directories(logDir_);

    // Open initial log file
    openNewLogFile();

    initialized_ = true;
  }

  // Set minimum log level
  void setLevel(LogLevel level) { minLevel_ = level; }

  // Log message
  void log(LogLevel level, const std::string &message,
           const std::string &source = "") {
    if (!initialized_ || level < minLevel_)
      return;

    std::lock_guard<std::mutex> lock(mutex_);

    // Format log entry
    std::ostringstream oss;
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif

    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "." << std::setw(3)
        << std::setfill('0') << ms.count() << " [" << logLevelStr(level) << "]";

    if (!source.empty()) {
      oss << " [" << source << "]";
    }

    oss << " " << message << "\n";

    std::string line = oss.str();

    // Write to file
    if (file_.is_open()) {
      file_ << line;
      file_.flush();
      currentSize_ += line.size();

      // Rotate if needed
      if (currentSize_ >= maxFileSize_) {
        rotateLog();
      }
    }

    // Also write to console if verbose
    if (consoleOutput_) {
      std::cout << line;
    }
  }

  // Convenience methods
  void trace(const std::string &msg, const std::string &src = "") {
    log(LogLevel::TRACE, msg, src);
  }
  void debug(const std::string &msg, const std::string &src = "") {
    log(LogLevel::DEBUG, msg, src);
  }
  void info(const std::string &msg, const std::string &src = "") {
    log(LogLevel::INFO, msg, src);
  }
  void warn(const std::string &msg, const std::string &src = "") {
    log(LogLevel::WARN, msg, src);
  }
  void error(const std::string &msg, const std::string &src = "") {
    log(LogLevel::ERROR, msg, src);
  }
  void fatal(const std::string &msg, const std::string &src = "") {
    log(LogLevel::FATAL, msg, src);
  }

  // Enable/disable console output
  void setConsoleOutput(bool enabled) { consoleOutput_ = enabled; }

  // Get current log file path
  std::string currentLogFile() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentFile_;
  }

  // Flush logs
  void flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open())
      file_.flush();
  }

  // Shutdown
  void shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
      file_.flush();
      file_.close();
    }
    initialized_ = false;
  }

private:
  RotatingLogger() = default;
  ~RotatingLogger() { shutdown(); }

  void openNewLogFile() {
    if (file_.is_open()) {
      file_.close();
    }

    // Generate filename with timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif

    std::ostringstream oss;
    oss << prefix_ << "_" << std::put_time(&tm_buf, "%Y%m%d_%H%M%S") << ".log";

    currentFile_ = (fs::path(logDir_) / oss.str()).string();
    file_.open(currentFile_, std::ios::app);
    currentSize_ = 0;

    if (file_.is_open()) {
      file_ << "=== Log started at "
            << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << " ===\n";
      file_.flush();
    }
  }

  void rotateLog() {
    openNewLogFile();
    cleanupOldLogs();
  }

  void cleanupOldLogs() {
    std::vector<fs::path> logFiles;

    try {
      for (const auto &entry : fs::directory_iterator(logDir_)) {
        if (entry.path().extension() == ".log" &&
            entry.path().filename().string().find(prefix_) == 0) {
          logFiles.push_back(entry.path());
        }
      }
    } catch (...) {
      return;
    }

    // Sort by modification time (newest first)
    std::sort(logFiles.begin(), logFiles.end(),
              [](const fs::path &a, const fs::path &b) {
                return fs::last_write_time(a) > fs::last_write_time(b);
              });

    // Remove old files
    while (logFiles.size() > static_cast<size_t>(maxFiles_)) {
      try {
        fs::remove(logFiles.back());
        logFiles.pop_back();
      } catch (...) {
        break;
      }
    }
  }

  mutable std::mutex mutex_;
  std::ofstream file_;
  std::string logDir_;
  std::string prefix_;
  std::string currentFile_;
  size_t maxFileSize_ = 10 * 1024 * 1024;
  size_t currentSize_ = 0;
  int maxFiles_ = 5;
  LogLevel minLevel_ = LogLevel::INFO;
  bool consoleOutput_ = false;
  bool initialized_ = false;
};

// =============================================================================
// CONVENIENCE MACROS
// =============================================================================
#define LOG_TRACE(msg) Hub::RotatingLogger::instance().trace(msg, __func__)
#define LOG_DEBUG(msg) Hub::RotatingLogger::instance().debug(msg, __func__)
#define LOG_INFO(msg) Hub::RotatingLogger::instance().info(msg, __func__)
#define LOG_WARN(msg) Hub::RotatingLogger::instance().warn(msg, __func__)
#define LOG_ERROR(msg) Hub::RotatingLogger::instance().error(msg, __func__)
#define LOG_FATAL(msg) Hub::RotatingLogger::instance().fatal(msg, __func__)

} // namespace Hub

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) void hub_log_init(const char *log_dir, const char *prefix,
                                        int max_size_mb, int max_files);
__declspec(dllexport) void hub_log_set_level(int level);
__declspec(dllexport) void hub_log_set_console(int enabled);
__declspec(dllexport) void hub_log(int level, const char *message,
                                   const char *source);
__declspec(dllexport) void hub_log_info(const char *message);
__declspec(dllexport) void hub_log_warn(const char *message);
__declspec(dllexport) void hub_log_error(const char *message);
__declspec(dllexport) const char *hub_log_current_file();
__declspec(dllexport) void hub_log_flush();
__declspec(dllexport) void hub_log_shutdown();
}
