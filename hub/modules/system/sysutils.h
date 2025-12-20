#pragma once
// =============================================================================
// SYSUTILS.H - System Utilities
// =============================================================================
// Additional system utilities:
// - Environment variables
// - Registry access (Windows)
// - Basic process info
// =============================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <optional>
#include <string>

namespace Hub::System {

// =============================================================================
// ENVIRONMENT VARIABLES
// =============================================================================
class Environment {
public:
  // Get environment variable
  static std::optional<std::string> get(const std::string &name) {
    char buffer[32767];
    DWORD result =
        GetEnvironmentVariableA(name.c_str(), buffer, sizeof(buffer));
    if (result == 0 || result >= sizeof(buffer)) {
      return std::nullopt;
    }
    return std::string(buffer);
  }

  // Set environment variable
  static bool set(const std::string &name, const std::string &value) {
    return SetEnvironmentVariableA(name.c_str(), value.c_str()) != 0;
  }

  // Remove environment variable
  static bool remove(const std::string &name) {
    return SetEnvironmentVariableA(name.c_str(), nullptr) != 0;
  }

  // Expand environment strings (e.g., %PATH%)
  static std::string expand(const std::string &str) {
    char buffer[32767];
    DWORD result =
        ExpandEnvironmentStringsA(str.c_str(), buffer, sizeof(buffer));
    if (result == 0 || result >= sizeof(buffer)) {
      return str;
    }
    return std::string(buffer);
  }

  // Get common paths
  static std::string userHome() {
    auto home = get("USERPROFILE");
    return home.value_or("");
  }

  static std::string tempDir() {
    char buffer[MAX_PATH];
    GetTempPathA(MAX_PATH, buffer);
    return std::string(buffer);
  }

  static std::string appData() {
    auto appdata = get("APPDATA");
    return appdata.value_or("");
  }

  static std::string systemRoot() {
    auto sysroot = get("SystemRoot");
    return sysroot.value_or("C:\\Windows");
  }
};

// =============================================================================
// REGISTRY ACCESS
// =============================================================================
class Registry {
public:
  // Read string value
  static std::optional<std::string>
  getString(HKEY root, const std::string &path, const std::string &name) {
    HKEY key;
    if (RegOpenKeyExA(root, path.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) {
      return std::nullopt;
    }

    char buffer[1024];
    DWORD size = sizeof(buffer);
    DWORD type;

    LONG result = RegQueryValueExA(key, name.c_str(), nullptr, &type,
                                   reinterpret_cast<LPBYTE>(buffer), &size);
    RegCloseKey(key);

    if (result != ERROR_SUCCESS || type != REG_SZ) {
      return std::nullopt;
    }

    return std::string(buffer);
  }

  // Read DWORD value
  static std::optional<uint32_t> getDword(HKEY root, const std::string &path,
                                          const std::string &name) {
    HKEY key;
    if (RegOpenKeyExA(root, path.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) {
      return std::nullopt;
    }

    DWORD value;
    DWORD size = sizeof(value);
    DWORD type;

    LONG result = RegQueryValueExA(key, name.c_str(), nullptr, &type,
                                   reinterpret_cast<LPBYTE>(&value), &size);
    RegCloseKey(key);

    if (result != ERROR_SUCCESS || type != REG_DWORD) {
      return std::nullopt;
    }

    return value;
  }

  // Write string value
  static bool setString(HKEY root, const std::string &path,
                        const std::string &name, const std::string &value) {
    HKEY key;
    DWORD disposition;
    if (RegCreateKeyExA(root, path.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr,
                        &key, &disposition) != ERROR_SUCCESS) {
      return false;
    }

    LONG result = RegSetValueExA(key, name.c_str(), 0, REG_SZ,
                                 reinterpret_cast<const BYTE *>(value.c_str()),
                                 static_cast<DWORD>(value.size() + 1));
    RegCloseKey(key);

    return result == ERROR_SUCCESS;
  }
};

// =============================================================================
// BASIC PROCESS INFO
// =============================================================================
class Process {
public:
  // Get current process ID
  static uint32_t currentId() { return GetCurrentProcessId(); }

  // Get current process path
  static std::string currentPath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::string(buffer);
  }

  // Check if process exists
  static bool exists(uint32_t pid) {
    HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (handle == nullptr) {
      return false;
    }
    CloseHandle(handle);
    return true;
  }

  // Get processor count
  static uint32_t cpuCount() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwNumberOfProcessors;
  }
};

} // namespace Hub::System

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
// Environment
__declspec(dllexport) const char *hub_env_get(const char *name);
__declspec(dllexport) int hub_env_set(const char *name, const char *value);
__declspec(dllexport) const char *hub_env_expand(const char *str);
__declspec(dllexport) const char *hub_env_home();
__declspec(dllexport) const char *hub_env_temp();
__declspec(dllexport) const char *hub_env_appdata();

// Process
__declspec(dllexport) unsigned int hub_process_id();
__declspec(dllexport) const char *hub_process_path();
__declspec(dllexport) int hub_process_exists(unsigned int pid);
__declspec(dllexport) unsigned int hub_cpu_count();
}
