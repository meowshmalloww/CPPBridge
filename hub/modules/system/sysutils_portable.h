#pragma once
// =============================================================================
// SYSUTILS_PORTABLE.H - Cross-Platform System Utilities
// =============================================================================
// Works on Windows, Linux, and macOS
// =============================================================================

#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define HUB_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#define HUB_PLATFORM_POSIX
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#endif

namespace Hub::System {

// =============================================================================
// CROSS-PLATFORM ENVIRONMENT
// =============================================================================
class EnvironmentPortable {
public:
  static std::optional<std::string> get(const std::string &name) {
    const char *value = std::getenv(name.c_str());
    if (value)
      return std::string(value);
    return std::nullopt;
  }

  static bool set(const std::string &name, const std::string &value) {
#ifdef HUB_PLATFORM_WINDOWS
    return SetEnvironmentVariableA(name.c_str(), value.c_str()) != 0;
#else
    return setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
  }

  static bool remove(const std::string &name) {
#ifdef HUB_PLATFORM_WINDOWS
    return SetEnvironmentVariableA(name.c_str(), nullptr) != 0;
#else
    return unsetenv(name.c_str()) == 0;
#endif
  }

  static std::string userHome() {
#ifdef HUB_PLATFORM_WINDOWS
    auto home = get("USERPROFILE");
#else
    auto home = get("HOME");
#endif
    return home.value_or("");
  }

  static std::string tempDir() {
#ifdef HUB_PLATFORM_WINDOWS
    char buffer[MAX_PATH];
    GetTempPathA(MAX_PATH, buffer);
    return std::string(buffer);
#else
    const char *tmp = std::getenv("TMPDIR");
    if (tmp)
      return tmp;
    return "/tmp";
#endif
  }
};

// =============================================================================
// CROSS-PLATFORM PROCESS
// =============================================================================
struct ProcessInfoPortable {
  uint32_t pid;
  std::string name;
};

class ProcessPortable {
public:
  static uint32_t currentId() {
#ifdef HUB_PLATFORM_WINDOWS
    return GetCurrentProcessId();
#else
    return static_cast<uint32_t>(getpid());
#endif
  }

  static uint32_t cpuCount() {
#ifdef HUB_PLATFORM_WINDOWS
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwNumberOfProcessors;
#else
    return static_cast<uint32_t>(sysconf(_SC_NPROCESSORS_ONLN));
#endif
  }

  static bool exists(uint32_t pid) {
#ifdef HUB_PLATFORM_WINDOWS
    HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (handle == nullptr)
      return false;
    CloseHandle(handle);
    return true;
#else
    return kill(static_cast<pid_t>(pid), 0) == 0;
#endif
  }

  static bool killProcess(uint32_t pid) {
#ifdef HUB_PLATFORM_WINDOWS
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!h)
      return false;
    BOOL result = TerminateProcess(h, 0);
    CloseHandle(h);
    return result != 0;
#else
    return kill(static_cast<pid_t>(pid), SIGTERM) == 0;
#endif
  }

  static uint32_t spawn(const std::string &command, bool hidden = false) {
#ifdef HUB_PLATFORM_WINDOWS
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    if (hidden) {
      si.dwFlags = STARTF_USESHOWWINDOW;
      si.wShowWindow = SW_HIDE;
    }

    PROCESS_INFORMATION pi = {};
    if (!CreateProcessA(nullptr, const_cast<char *>(command.c_str()), nullptr,
                        nullptr, FALSE, hidden ? CREATE_NO_WINDOW : 0, nullptr,
                        nullptr, &si, &pi)) {
      return 0;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return pi.dwProcessId;
#else
    (void)hidden;
    pid_t pid = fork();
    if (pid == 0) {
      // Child process
      execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
      _exit(127);
    }
    return static_cast<uint32_t>(pid);
#endif
  }

  static std::vector<ProcessInfoPortable> list() {
    std::vector<ProcessInfoPortable> result;

#ifdef HUB_PLATFORM_WINDOWS
    for (DWORD pid = 4; pid < 65536; pid += 4) {
      HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
      if (h) {
        ProcessInfoPortable info;
        info.pid = pid;

        char path[MAX_PATH] = {0};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameA(h, 0, path, &size)) {
          const char *name = strrchr(path, '\\');
          info.name = name ? (name + 1) : path;
        }

        CloseHandle(h);
        result.push_back(std::move(info));
      }
    }
#else
    // Linux: read /proc
    DIR *dir = opendir("/proc");
    if (!dir)
      return result;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
      // Check if directory name is a number (PID)
      char *end;
      long pid = strtol(entry->d_name, &end, 10);
      if (*end != '\0' || pid <= 0)
        continue;

      ProcessInfoPortable info;
      info.pid = static_cast<uint32_t>(pid);

      // Read process name from /proc/[pid]/comm
      std::string commPath = "/proc/" + std::string(entry->d_name) + "/comm";
      std::ifstream commFile(commPath);
      if (commFile) {
        std::getline(commFile, info.name);
      }

      result.push_back(std::move(info));
    }
    closedir(dir);
#endif

    return result;
  }
};

} // namespace Hub::System

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
#ifdef HUB_PLATFORM_WINDOWS
#define HUB_EXPORT __declspec(dllexport)
#else
#define HUB_EXPORT __attribute__((visibility("default")))
#endif

HUB_EXPORT const char *hub_env_portable_get(const char *name);
HUB_EXPORT int hub_env_portable_set(const char *name, const char *value);
HUB_EXPORT const char *hub_env_portable_home();
HUB_EXPORT const char *hub_env_portable_temp();
HUB_EXPORT unsigned int hub_proc_portable_id();
HUB_EXPORT unsigned int hub_proc_portable_cpu_count();
HUB_EXPORT int hub_proc_portable_exists(unsigned int pid);
HUB_EXPORT int hub_proc_portable_kill(unsigned int pid);
HUB_EXPORT unsigned int hub_proc_portable_spawn(const char *command,
                                                int hidden);
}
