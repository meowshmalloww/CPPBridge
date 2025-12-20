// =============================================================================
// CRASHHANDLER_ADV.CPP - Advanced Crash Handler Implementation
// =============================================================================
// Uses basic Windows APIs without dbghelp.h to avoid SDK conflicts
// Stack traces use RtlCaptureStackBackTrace
// =============================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

#include "crashhandler_adv.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace Hub::Crash {

static std::mutex g_crashMutex;

// =============================================================================
// STACK TRACE CAPTURE (without dbghelp)
// =============================================================================
namespace detail {

std::string resolveSymbol(uint64_t address) {
  // Without dbghelp, we can only return the address
  std::ostringstream oss;
  oss << "0x" << std::hex << address;
  return oss.str();
}

std::vector<StackFrame> captureStackTrace(void * /*contextPtr*/) {
  std::vector<StackFrame> frames;

  // Use RtlCaptureStackBackTrace
  void *stack[64];
  USHORT count = CaptureStackBackTrace(0, 64, stack, nullptr);

  for (USHORT i = 0; i < count; ++i) {
    StackFrame frame;
    frame.address = reinterpret_cast<uint64_t>(stack[i]);

    // Get module name
    HMODULE module = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                           static_cast<LPCSTR>(stack[i]), &module)) {
      char moduleName[MAX_PATH];
      if (GetModuleFileNameA(module, moduleName, MAX_PATH)) {
        // Extract just the filename
        std::string path = moduleName;
        auto pos = path.rfind('\\');
        if (pos != std::string::npos) {
          frame.moduleName = path.substr(pos + 1);
        } else {
          frame.moduleName = path;
        }
      }
      FreeLibrary(module);
    }

    frame.functionName = resolveSymbol(frame.address);
    frames.push_back(frame);
  }

  return frames;
}

bool writeMiniDump(const std::string &path, void *exceptionPointers,
                   bool /*fullMemory*/) {
  // Write a simple crash dump file (not a full minidump without dbghelp)
  std::ofstream file(path, std::ios::binary);
  if (!file)
    return false;

  // Write header
  file << "CRASH DUMP\n";
  file << "Time: " << std::time(nullptr) << "\n";
  file << "Process: " << GetCurrentProcessId() << "\n";
  file << "Thread: " << GetCurrentThreadId() << "\n";

  if (exceptionPointers) {
    auto *ex = static_cast<PEXCEPTION_POINTERS>(exceptionPointers);
    file << "Exception Code: 0x" << std::hex
         << ex->ExceptionRecord->ExceptionCode << "\n";
    file << "Exception Address: 0x" << std::hex
         << reinterpret_cast<uint64_t>(ex->ExceptionRecord->ExceptionAddress)
         << "\n";
  }

  // Write stack trace
  file << "\nStack Trace:\n";
  auto stack = captureStackTrace(nullptr);
  for (const auto &frame : stack) {
    file << "  " << frame.moduleName << "!" << frame.functionName << "\n";
  }

  file.close();
  return true;
}

} // namespace detail

// =============================================================================
// EXCEPTION FILTER
// =============================================================================
static LONG WINAPI UnhandledExceptionFilter(PEXCEPTION_POINTERS exceptionInfo) {
  std::lock_guard<std::mutex> lock(g_crashMutex);

  auto &handler = CrashHandler::instance();
  CrashInfo info;

  info.processId = GetCurrentProcessId();
  info.threadId = GetCurrentThreadId();
  info.exceptionCode = exceptionInfo->ExceptionRecord->ExceptionCode;
  info.exceptionAddress = reinterpret_cast<uint64_t>(
      exceptionInfo->ExceptionRecord->ExceptionAddress);

  // Exception message
  switch (info.exceptionCode) {
  case EXCEPTION_ACCESS_VIOLATION:
    info.exceptionMessage = "Access Violation";
    break;
  case EXCEPTION_STACK_OVERFLOW:
    info.exceptionMessage = "Stack Overflow";
    break;
  case EXCEPTION_INT_DIVIDE_BY_ZERO:
    info.exceptionMessage = "Division by Zero";
    break;
  case EXCEPTION_ILLEGAL_INSTRUCTION:
    info.exceptionMessage = "Illegal Instruction";
    break;
  default: {
    std::ostringstream oss;
    oss << "Exception 0x" << std::hex << info.exceptionCode;
    info.exceptionMessage = oss.str();
  }
  }

  // Timestamp
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&time), "%Y%m%d_%H%M%S");
  info.timestamp = oss.str();

  // Capture stack trace
  info.stackTrace = detail::captureStackTrace(nullptr);

  // Write dump
  std::string dumpPath = handler.lastCrash().minidumpPath;
  if (dumpPath.empty()) {
    char temp[MAX_PATH];
    GetTempPathA(MAX_PATH, temp);
    dumpPath = temp;
  }

  dumpPath += "\\crash_" + info.timestamp + "_" +
              std::to_string(info.processId) + ".dmp";

  if (detail::writeMiniDump(dumpPath, exceptionInfo, false)) {
    info.minidumpPath = dumpPath;
  }

  // Store last crash
  const_cast<CrashInfo &>(handler.lastCrash()) = info;

  return EXCEPTION_EXECUTE_HANDLER;
}

// =============================================================================
// CRASH HANDLER IMPLEMENTATION
// =============================================================================
CrashHandler &CrashHandler::instance() {
  static CrashHandler instance;
  return instance;
}

CrashHandler::~CrashHandler() { shutdown(); }

bool CrashHandler::init(const CrashConfig &config) {
  std::lock_guard<std::mutex> lock(g_crashMutex);

  if (initialized_) {
    return true;
  }

  config_ = config;

  if (config_.dumpDirectory.empty()) {
    char temp[MAX_PATH];
    GetTempPathA(MAX_PATH, temp);
    config_.dumpDirectory = temp;
  }

  // Create dump directory
  std::filesystem::create_directories(config_.dumpDirectory);

  // Set exception handler
  exceptionHandler_ = AddVectoredExceptionHandler(1, UnhandledExceptionFilter);
  SetUnhandledExceptionFilter(UnhandledExceptionFilter);

  initialized_ = true;
  return true;
}

void CrashHandler::shutdown() {
  std::lock_guard<std::mutex> lock(g_crashMutex);

  if (!initialized_)
    return;

  if (exceptionHandler_) {
    RemoveVectoredExceptionHandler(exceptionHandler_);
    exceptionHandler_ = nullptr;
  }

  initialized_ = false;
}

void CrashHandler::setCallback(CrashCallback cb) { callback_ = std::move(cb); }

void CrashHandler::reportCrash(const std::string &message) {
  CrashInfo info;
  info.processId = GetCurrentProcessId();
  info.threadId = GetCurrentThreadId();
  info.exceptionMessage = message;

  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&time), "%Y%m%d_%H%M%S");
  info.timestamp = oss.str();

  info.stackTrace = detail::captureStackTrace(nullptr);

  std::string dumpPath =
      config_.dumpDirectory + "\\manual_" + info.timestamp + ".dmp";
  if (detail::writeMiniDump(dumpPath, nullptr, false)) {
    info.minidumpPath = dumpPath;
  }

  lastCrash_ = info;

  if (callback_) {
    callback_(info);
  }
}

bool CrashHandler::uploadDump(const std::string &dumpPath) {
  if (config_.uploadUrl.empty()) {
    return false;
  }

  // Read dump file
  std::ifstream file(dumpPath, std::ios::binary);
  if (!file)
    return false;

  std::vector<char> data((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
  file.close();

  // Parse URL
  std::wstring url(config_.uploadUrl.begin(), config_.uploadUrl.end());

  // Use WinHTTP to upload
  HINTERNET session =
      WinHttpOpen(L"UniversalBridge/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!session)
    return false;

  URL_COMPONENTS urlComp = {};
  urlComp.dwStructSize = sizeof(urlComp);
  wchar_t hostName[256] = {};
  wchar_t urlPath[1024] = {};
  urlComp.lpszHostName = hostName;
  urlComp.dwHostNameLength = 256;
  urlComp.lpszUrlPath = urlPath;
  urlComp.dwUrlPathLength = 1024;

  if (!WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp)) {
    WinHttpCloseHandle(session);
    return false;
  }

  HINTERNET connect = WinHttpConnect(session, hostName, urlComp.nPort, 0);
  if (!connect) {
    WinHttpCloseHandle(session);
    return false;
  }

  DWORD flags =
      (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
  HINTERNET request =
      WinHttpOpenRequest(connect, L"POST", urlPath, nullptr, WINHTTP_NO_REFERER,
                         WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
  if (!request) {
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return false;
  }

  LPCWSTR headers = L"Content-Type: application/octet-stream";
  BOOL sent = WinHttpSendRequest(request, headers, static_cast<DWORD>(-1),
                                 data.data(), static_cast<DWORD>(data.size()),
                                 static_cast<DWORD>(data.size()), 0);

  bool success = false;
  if (sent && WinHttpReceiveResponse(request, nullptr)) {
    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);
    WinHttpQueryHeaders(request,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        nullptr, &statusCode, &size, nullptr);
    success = (statusCode >= 200 && statusCode < 300);
  }

  WinHttpCloseHandle(request);
  WinHttpCloseHandle(connect);
  WinHttpCloseHandle(session);

  return success;
}

const CrashInfo &CrashHandler::lastCrash() const { return lastCrash_; }

std::vector<std::string> CrashHandler::listDumps() const {
  std::vector<std::string> dumps;

  if (config_.dumpDirectory.empty())
    return dumps;

  try {
    for (const auto &entry :
         std::filesystem::directory_iterator(config_.dumpDirectory)) {
      if (entry.path().extension() == ".dmp") {
        dumps.push_back(entry.path().string());
      }
    }
  } catch (...) {
    // Directory might not exist
  }

  return dumps;
}

size_t CrashHandler::cleanupDumps(int maxAgeDays) {
  size_t removed = 0;
  auto now = std::filesystem::file_time_type::clock::now();
  auto maxAge = std::chrono::hours(24 * maxAgeDays);

  try {
    for (const auto &entry :
         std::filesystem::directory_iterator(config_.dumpDirectory)) {
      if (entry.path().extension() == ".dmp") {
        auto age = now - entry.last_write_time();
        if (age > maxAge) {
          std::filesystem::remove(entry.path());
          removed++;
        }
      }
    }
  } catch (...) {
    // Ignore errors
  }

  return removed;
}

} // namespace Hub::Crash

// =============================================================================
// C API
// =============================================================================
static thread_local std::string tl_last_dump;

// Forward declare EnumProcessModules
extern "C" BOOL WINAPI EnumProcessModules(HANDLE, HMODULE *, DWORD, LPDWORD);
#pragma comment(lib, "psapi.lib")

extern "C" {

__declspec(dllexport) int hub_crash_adv_init(const char *dump_dir,
                                             const char *upload_url) {
  Hub::Crash::CrashConfig config;
  if (dump_dir)
    config.dumpDirectory = dump_dir;
  if (upload_url)
    config.uploadUrl = upload_url;
  return Hub::Crash::CrashHandler::instance().init(config) ? 1 : 0;
}

__declspec(dllexport) void hub_crash_adv_shutdown() {
  Hub::Crash::CrashHandler::instance().shutdown();
}

__declspec(dllexport) void hub_crash_adv_report(const char *message) {
  Hub::Crash::CrashHandler::instance().reportCrash(
      message ? message : "Manual crash report");
}

__declspec(dllexport) int hub_crash_adv_upload(const char *dump_path) {
  if (!dump_path)
    return 0;
  return Hub::Crash::CrashHandler::instance().uploadDump(dump_path) ? 1 : 0;
}

__declspec(dllexport) const char *hub_crash_adv_last_dump_path() {
  tl_last_dump = Hub::Crash::CrashHandler::instance().lastCrash().minidumpPath;
  return tl_last_dump.c_str();
}

__declspec(dllexport) int hub_crash_adv_dump_count() {
  return static_cast<int>(
      Hub::Crash::CrashHandler::instance().listDumps().size());
}

__declspec(dllexport) int hub_crash_adv_cleanup(int max_age_days) {
  return static_cast<int>(
      Hub::Crash::CrashHandler::instance().cleanupDumps(max_age_days));
}

} // extern "C"
