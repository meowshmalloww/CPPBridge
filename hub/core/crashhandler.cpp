// =============================================================================
// CRASHHANDLER.CPP - Crash Handler Implementation
// =============================================================================

#include "crashhandler.h"

static thread_local std::string tl_dump_dir;
static thread_local std::string tl_last_log;
static thread_local std::string tl_last_message;
static thread_local std::string tl_report_path;

extern "C" {

__declspec(dllexport) void hub_crash_init(const char *dump_dir) {
  Hub::CrashHandler::instance().init(dump_dir ? dump_dir : "crashdumps");
}

__declspec(dllexport) void hub_crash_shutdown() {
  Hub::CrashHandler::instance().shutdown();
}

__declspec(dllexport) const char *hub_crash_dump_dir() {
  tl_dump_dir = Hub::CrashHandler::instance().dumpDirectory();
  return tl_dump_dir.c_str();
}

__declspec(dllexport) const char *hub_crash_last_log() {
  tl_last_log = Hub::CrashHandler::instance().lastCrash().logFile;
  return tl_last_log.c_str();
}

__declspec(dllexport) const char *hub_crash_last_message() {
  tl_last_message = Hub::CrashHandler::instance().lastCrash().message;
  return tl_last_message.c_str();
}

__declspec(dllexport) const char *hub_crash_write_report(const char *message) {
  tl_report_path = Hub::CrashHandler::instance().writeReport(
      message ? message : "Manual report");
  return tl_report_path.c_str();
}

} // extern "C"
