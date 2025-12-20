#pragma once
// =============================================================================
// CRASHHANDLER.H - Crash Reporting
// =============================================================================
// Simplified crash handler using Windows Structured Exception Handling.
// Captures exception info and writes crash logs.
// =============================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

namespace Hub {

namespace fs = std::filesystem;

// =============================================================================
// CRASH INFO
// =============================================================================
struct CrashInfo {
  DWORD exceptionCode = 0;
  void *exceptionAddress = nullptr;
  std::string message;
  std::string logFile;
  std::chrono::system_clock::time_point timestamp;
};

// =============================================================================
// CRASH HANDLER
// =============================================================================
class CrashHandler {
public:
  static CrashHandler &instance() {
    static CrashHandler inst;
    return inst;
  }

  // Initialize crash handling
  void init(const std::string &dumpDir = "crashdumps") {
    if (initialized_)
      return;

    dumpDir_ = dumpDir;
    fs::create_directories(dumpDir_);

    // Install exception handler
    prevHandler_ = SetUnhandledExceptionFilter(exceptionFilter);

    initialized_ = true;
  }

  // Shutdown
  void shutdown() {
    if (!initialized_)
      return;

    if (prevHandler_) {
      SetUnhandledExceptionFilter(prevHandler_);
      prevHandler_ = nullptr;
    }

    initialized_ = false;
  }

  // Set crash callback
  using CrashCallback = void (*)(const char *logPath, const char *message);
  void setCallback(CrashCallback callback) { callback_ = callback; }

  // Get last crash info
  const CrashInfo &lastCrash() const { return lastCrash_; }

  // Get dump directory
  const std::string &dumpDirectory() const { return dumpDir_; }

  // Write manual crash report
  std::string writeReport(const std::string &message, DWORD code = 0,
                          void *addr = nullptr) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
    localtime_s(&tm_buf, &time);

    std::ostringstream filename;
    filename << "crash_" << std::put_time(&tm_buf, "%Y%m%d_%H%M%S") << ".log";

    std::string logPath = (fs::path(dumpDir_) / filename.str()).string();

    std::ofstream log(logPath);
    if (log.is_open()) {
      log << "=== CRASH REPORT ===\n";
      log << "Time: " << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "\n";
      log << "Exception: 0x" << std::hex << code << "\n";
      log << "Message: " << message << "\n";
      log << "Address: " << addr << "\n";
      log << "\n=== SYSTEM INFO ===\n";
      log << "Process ID: " << GetCurrentProcessId() << "\n";
      log << "Thread ID: " << GetCurrentThreadId() << "\n";
    }

    return logPath;
  }

private:
  CrashHandler() = default;
  ~CrashHandler() { shutdown(); }

  static LONG WINAPI exceptionFilter(EXCEPTION_POINTERS *exInfo) {
    auto &handler = instance();

    // Build crash info
    handler.lastCrash_.exceptionCode = exInfo->ExceptionRecord->ExceptionCode;
    handler.lastCrash_.exceptionAddress =
        exInfo->ExceptionRecord->ExceptionAddress;
    handler.lastCrash_.timestamp = std::chrono::system_clock::now();
    handler.lastCrash_.message =
        getExceptionName(exInfo->ExceptionRecord->ExceptionCode);

    // Write crash log
    handler.lastCrash_.logFile = handler.writeReport(
        handler.lastCrash_.message, handler.lastCrash_.exceptionCode,
        handler.lastCrash_.exceptionAddress);

    // Call callback
    if (handler.callback_) {
      handler.callback_(handler.lastCrash_.logFile.c_str(),
                        handler.lastCrash_.message.c_str());
    }

    // Continue to previous handler or terminate
    if (handler.prevHandler_) {
      return handler.prevHandler_(exInfo);
    }

    return EXCEPTION_EXECUTE_HANDLER;
  }

  static const char *getExceptionName(DWORD code) {
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
      return "ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      return "ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT:
      return "BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT:
      return "DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      return "FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_OVERFLOW:
      return "FLT_OVERFLOW";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      return "ILLEGAL_INSTRUCTION";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      return "INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW:
      return "INT_OVERFLOW";
    case EXCEPTION_STACK_OVERFLOW:
      return "STACK_OVERFLOW";
    default:
      return "UNKNOWN_EXCEPTION";
    }
  }

  std::string dumpDir_;
  LPTOP_LEVEL_EXCEPTION_FILTER prevHandler_ = nullptr;
  CrashCallback callback_ = nullptr;
  CrashInfo lastCrash_;
  bool initialized_ = false;
};

} // namespace Hub

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) void hub_crash_init(const char *dump_dir);
__declspec(dllexport) void hub_crash_shutdown();
__declspec(dllexport) const char *hub_crash_dump_dir();
__declspec(dllexport) const char *hub_crash_last_log();
__declspec(dllexport) const char *hub_crash_last_message();
__declspec(dllexport) const char *hub_crash_write_report(const char *message);
}
