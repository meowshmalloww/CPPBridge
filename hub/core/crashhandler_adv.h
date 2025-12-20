#pragma once
// =============================================================================
// CRASHHANDLER_ADV.H - Advanced Crash Handler with Minidumps
// =============================================================================
// Production-grade crash handling:
// - Full minidump with stack traces (using dbghelp.h)
// - Crash upload to server
// - Symbol resolution
// - Exception information capture
// =============================================================================

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Hub::Crash {

// =============================================================================
// CRASH INFO
// =============================================================================
struct StackFrame {
  uint64_t address;
  std::string moduleName;
  std::string functionName;
  std::string fileName;
  uint32_t lineNumber;
};

struct CrashInfo {
  uint32_t processId;
  uint32_t threadId;
  uint64_t exceptionCode;
  uint64_t exceptionAddress;
  std::string exceptionMessage;
  std::vector<StackFrame> stackTrace;
  std::string minidumpPath;
  std::string timestamp;
};

// =============================================================================
// CRASH HANDLER CONFIGURATION
// =============================================================================
struct CrashConfig {
  std::string dumpDirectory;      // Where to save minidumps
  std::string uploadUrl;          // Server URL for crash upload
  bool uploadOnCrash = false;     // Auto-upload on crash
  bool includeFullMemory = false; // Full memory dump (large files)
  bool collectStackTrace = true;  // Collect stack trace
  bool showDialog = true;         // Show crash dialog to user
};

// =============================================================================
// CRASH HANDLER
// =============================================================================
class CrashHandler {
public:
  using CrashCallback = std::function<void(const CrashInfo &)>;

  static CrashHandler &instance();

  // Initialize crash handler
  bool init(const CrashConfig &config = {});

  // Shutdown
  void shutdown();

  // Set crash callback
  void setCallback(CrashCallback cb);

  // Manual crash report
  void reportCrash(const std::string &message);

  // Upload crash dump to server
  bool uploadDump(const std::string &dumpPath);

  // Get last crash info
  const CrashInfo &lastCrash() const;

  // List all crash dumps
  std::vector<std::string> listDumps() const;

  // Delete old dumps
  size_t cleanupDumps(int maxAgeDays = 30);

private:
  CrashHandler() = default;
  ~CrashHandler();

  CrashConfig config_;
  CrashCallback callback_;
  CrashInfo lastCrash_;
  bool initialized_ = false;
  void *exceptionHandler_ = nullptr;
};

// Forward declaration for dbghelp-specific implementation
namespace detail {
bool writeMiniDump(const std::string &path, void *exceptionPointers,
                   bool fullMemory);
std::vector<StackFrame> captureStackTrace(void *context);
std::string resolveSymbol(uint64_t address);
} // namespace detail

} // namespace Hub::Crash

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_crash_adv_init(const char *dump_dir,
                                             const char *upload_url);
__declspec(dllexport) void hub_crash_adv_shutdown();
__declspec(dllexport) void hub_crash_adv_report(const char *message);
__declspec(dllexport) int hub_crash_adv_upload(const char *dump_path);
__declspec(dllexport) const char *hub_crash_adv_last_dump_path();
__declspec(dllexport) int hub_crash_adv_dump_count();
__declspec(dllexport) int hub_crash_adv_cleanup(int max_age_days);
}
