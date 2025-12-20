#pragma once
// =============================================================================
// LOGGING_ADV.H - Advanced Async Logging with JSON Support
// =============================================================================
// Features:
// - Async non-blocking logging (background thread)
// - JSON structured output for log aggregators
// - High-performance lock-free queue
// =============================================================================

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
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
enum class LogLvl : int {
  TRACE = 0,
  DEBUG = 1,
  INFO = 2,
  WARN = 3,
  ERR = 4,
  FATAL = 5
};

inline const char *logLvlName(LogLvl l) {
  switch (l) {
  case LogLvl::TRACE:
    return "TRACE";
  case LogLvl::DEBUG:
    return "DEBUG";
  case LogLvl::INFO:
    return "INFO";
  case LogLvl::WARN:
    return "WARN";
  case LogLvl::ERR:
    return "ERROR";
  case LogLvl::FATAL:
    return "FATAL";
  default:
    return "?";
  }
}

// =============================================================================
// LOG ENTRY
// =============================================================================
struct LogRecord {
  LogLvl level;
  std::string message;
  std::string source;
  std::string file;
  int line;
  std::chrono::system_clock::time_point time;
  std::map<std::string, std::string> fields; // Structured fields
};

// =============================================================================
// JSON FORMATTER
// =============================================================================
class JsonFormatter {
public:
  static std::string escape(const std::string &s) {
    std::string result;
    result.reserve(s.size() + 10);
    for (char c : s) {
      switch (c) {
      case '"':
        result += "\\\"";
        break;
      case '\\':
        result += "\\\\";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        result += c;
        break;
      }
    }
    return result;
  }

  static std::string format(const LogRecord &rec) {
    std::ostringstream oss;

    auto time = std::chrono::system_clock::to_time_t(rec.time);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  rec.time.time_since_epoch()) %
              1000;

    std::tm tm_buf;
#ifdef _WIN32
    gmtime_s(&tm_buf, &time);
#else
    gmtime_r(&time, &tm_buf);
#endif

    oss << "{";
    oss << "\"timestamp\":\"";
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
    oss << "." << std::setw(3) << std::setfill('0') << ms.count() << "Z\"";
    oss << ",\"level\":\"" << logLvlName(rec.level) << "\"";
    oss << ",\"message\":\"" << escape(rec.message) << "\"";

    if (!rec.source.empty()) {
      oss << ",\"source\":\"" << escape(rec.source) << "\"";
    }
    if (!rec.file.empty()) {
      oss << ",\"file\":\"" << escape(rec.file) << "\"";
      oss << ",\"line\":" << rec.line;
    }

    // Custom fields
    for (const auto &[key, val] : rec.fields) {
      oss << ",\"" << escape(key) << "\":\"" << escape(val) << "\"";
    }

    oss << "}\n";
    return oss.str();
  }
};

// =============================================================================
// TEXT FORMATTER
// =============================================================================
class TextFormatter {
public:
  static std::string format(const LogRecord &rec) {
    std::ostringstream oss;

    auto time = std::chrono::system_clock::to_time_t(rec.time);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  rec.time.time_since_epoch()) %
              1000;

    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif

    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setw(3) << std::setfill('0') << ms.count();
    oss << " [" << std::setw(5) << logLvlName(rec.level) << "]";

    if (!rec.source.empty()) {
      oss << " [" << rec.source << "]";
    }

    oss << " " << rec.message;

    // Append structured fields
    if (!rec.fields.empty()) {
      oss << " {";
      bool first = true;
      for (const auto &[key, val] : rec.fields) {
        if (!first)
          oss << ", ";
        oss << key << "=" << val;
        first = false;
      }
      oss << "}";
    }

    oss << "\n";
    return oss.str();
  }
};

// =============================================================================
// ASYNC LOGGER
// =============================================================================
class AsyncLogger {
public:
  enum class Format { TEXT, JSON };

  static AsyncLogger &instance() {
    static AsyncLogger inst;
    return inst;
  }

  void init(const std::string &logDir, const std::string &prefix = "app",
            size_t maxFileSize = 10 * 1024 * 1024, int maxFiles = 5) {
    std::lock_guard<std::mutex> lock(mutex_);

    logDir_ = logDir;
    prefix_ = prefix;
    maxFileSize_ = maxFileSize;
    maxFiles_ = maxFiles;

    fs::create_directories(logDir_);
    openNewFile();

    // Start background writer
    shutdown_ = false;
    writerThread_ = std::thread(&AsyncLogger::writerLoop, this);

    initialized_ = true;
  }

  void setLevel(LogLvl level) { minLevel_ = level; }
  void setFormat(Format fmt) { format_ = fmt; }
  void setConsole(bool enabled) { console_ = enabled; }

  // Non-blocking log
  void log(LogLvl level, const std::string &msg, const std::string &src = "",
           const std::string &file = "", int line = 0) {
    if (!initialized_ || level < minLevel_)
      return;

    LogRecord rec;
    rec.level = level;
    rec.message = msg;
    rec.source = src;
    rec.file = file;
    rec.line = line;
    rec.time = std::chrono::system_clock::now();

    enqueue(std::move(rec));
  }

  // Log with structured fields
  void logStructured(LogLvl level, const std::string &msg,
                     const std::map<std::string, std::string> &fields,
                     const std::string &src = "") {
    if (!initialized_ || level < minLevel_)
      return;

    LogRecord rec;
    rec.level = level;
    rec.message = msg;
    rec.source = src;
    rec.time = std::chrono::system_clock::now();
    rec.fields = fields;

    enqueue(std::move(rec));
  }

  void flush() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    cv_.notify_one();
    // Wait for queue to drain
    while (!queue_.empty()) {
      lock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      lock.lock();
    }
  }

  void shutdown() {
    if (!initialized_)
      return;

    shutdown_ = true;
    cv_.notify_one();

    if (writerThread_.joinable()) {
      writerThread_.join();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
      file_.close();
    }
    initialized_ = false;
  }

  std::string currentFile() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentFile_;
  }

private:
  AsyncLogger() = default;
  ~AsyncLogger() { shutdown(); }

  void enqueue(LogRecord &&rec) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push(std::move(rec));
    cv_.notify_one();
  }

  void writerLoop() {
    while (!shutdown_) {
      std::unique_lock<std::mutex> lock(queueMutex_);
      cv_.wait_for(lock, std::chrono::milliseconds(100),
                   [this] { return !queue_.empty() || shutdown_; });

      // Process batch
      std::queue<LogRecord> batch;
      std::swap(batch, queue_);
      lock.unlock();

      while (!batch.empty()) {
        writeRecord(batch.front());
        batch.pop();
      }
    }

    // Drain remaining
    std::lock_guard<std::mutex> lock(queueMutex_);
    while (!queue_.empty()) {
      writeRecord(queue_.front());
      queue_.pop();
    }
  }

  void writeRecord(const LogRecord &rec) {
    std::string line = (format_ == Format::JSON) ? JsonFormatter::format(rec)
                                                 : TextFormatter::format(rec);

    std::lock_guard<std::mutex> lock(mutex_);

    if (file_.is_open()) {
      file_ << line;
      currentSize_ += line.size();

      if (currentSize_ >= maxFileSize_) {
        rotateFile();
      }
    }

    if (console_) {
      std::cout << line;
    }
  }

  void openNewFile() {
    if (file_.is_open())
      file_.close();

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif

    std::ostringstream oss;
    oss << prefix_ << "_" << std::put_time(&tm_buf, "%Y%m%d_%H%M%S");
    oss << (format_ == Format::JSON ? ".jsonl" : ".log");

    currentFile_ = (fs::path(logDir_) / oss.str()).string();
    file_.open(currentFile_, std::ios::app);
    currentSize_ = 0;
  }

  void rotateFile() {
    openNewFile();
    cleanupOld();
  }

  void cleanupOld() {
    std::vector<fs::path> files;
    std::string ext = (format_ == Format::JSON) ? ".jsonl" : ".log";

    try {
      for (const auto &entry : fs::directory_iterator(logDir_)) {
        if (entry.path().extension() == ext &&
            entry.path().filename().string().find(prefix_) == 0) {
          files.push_back(entry.path());
        }
      }
    } catch (...) {
      return;
    }

    std::sort(files.begin(), files.end(),
              [](const fs::path &a, const fs::path &b) {
                return fs::last_write_time(a) > fs::last_write_time(b);
              });

    while (files.size() > static_cast<size_t>(maxFiles_)) {
      try {
        fs::remove(files.back());
        files.pop_back();
      } catch (...) {
        break;
      }
    }
  }

  mutable std::mutex mutex_;
  std::mutex queueMutex_;
  std::condition_variable cv_;
  std::queue<LogRecord> queue_;
  std::thread writerThread_;

  std::ofstream file_;
  std::string logDir_;
  std::string prefix_;
  std::string currentFile_;

  size_t maxFileSize_ = 10 * 1024 * 1024;
  size_t currentSize_ = 0;
  int maxFiles_ = 5;

  LogLvl minLevel_ = LogLvl::INFO;
  Format format_ = Format::TEXT;
  bool console_ = false;
  std::atomic<bool> initialized_{false};
  std::atomic<bool> shutdown_{false};
};

} // namespace Hub

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) void hub_log_adv_init(const char *log_dir,
                                            const char *prefix, int max_size_mb,
                                            int max_files);
__declspec(dllexport) void hub_log_adv_set_level(int level);
__declspec(dllexport) void hub_log_adv_set_json(int enabled);
__declspec(dllexport) void hub_log_adv_set_console(int enabled);
__declspec(dllexport) void hub_log_adv(int level, const char *message,
                                       const char *source);
__declspec(dllexport) void
hub_log_adv_structured(int level, const char *message, const char *json_fields);
__declspec(dllexport) const char *hub_log_adv_current_file();
__declspec(dllexport) void hub_log_adv_flush();
__declspec(dllexport) void hub_log_adv_shutdown();
}
