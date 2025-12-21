#pragma once
// =============================================================================
// SYSUTILS_ADV.H - Advanced System Utilities
// =============================================================================
// Features:
// - Registry binary (REG_BINARY) read/write
// - Registry key/value enumeration
// - Process listing (using NtQuerySystemInformation to avoid tlhelp32.h)
// - Child process spawn/kill/wait
// =============================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Hub::System {

// =============================================================================
// ADVANCED REGISTRY
// =============================================================================
class RegistryAdv {
public:
  // Read binary data
  static std::optional<std::vector<uint8_t>>
  getBinary(HKEY root, const std::string &path, const std::string &name) {
    HKEY key;
    if (RegOpenKeyExA(root, path.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) {
      return std::nullopt;
    }

    // First query size
    DWORD size = 0;
    DWORD type;
    RegQueryValueExA(key, name.c_str(), nullptr, &type, nullptr, &size);

    if (size == 0) {
      RegCloseKey(key);
      return std::nullopt;
    }

    std::vector<uint8_t> buffer(size);
    LONG result = RegQueryValueExA(key, name.c_str(), nullptr, &type,
                                   buffer.data(), &size);
    RegCloseKey(key);

    if (result != ERROR_SUCCESS || type != REG_BINARY) {
      return std::nullopt;
    }

    buffer.resize(size);
    return buffer;
  }

  // Write binary data
  static bool setBinary(HKEY root, const std::string &path,
                        const std::string &name, const uint8_t *data,
                        size_t len) {
    HKEY key;
    DWORD disposition;
    if (RegCreateKeyExA(root, path.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr,
                        &key, &disposition) != ERROR_SUCCESS) {
      return false;
    }

    LONG result = RegSetValueExA(key, name.c_str(), 0, REG_BINARY, data,
                                 static_cast<DWORD>(len));
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
  }

  static bool setBinary(HKEY root, const std::string &path,
                        const std::string &name,
                        const std::vector<uint8_t> &data) {
    return setBinary(root, path, name, data.data(), data.size());
  }

  // Enumerate subkeys
  static std::vector<std::string> enumKeys(HKEY root, const std::string &path) {
    std::vector<std::string> result;

    HKEY key;
    if (RegOpenKeyExA(root, path.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) {
      return result;
    }

    char name[256];
    DWORD index = 0;

    while (true) {
      DWORD nameLen = sizeof(name);
      LONG err = RegEnumKeyExA(key, index, name, &nameLen, nullptr, nullptr,
                               nullptr, nullptr);
      if (err != ERROR_SUCCESS)
        break;
      result.push_back(std::string(name, nameLen));
      index++;
    }

    RegCloseKey(key);
    return result;
  }

  // Enumerate values
  struct RegValue {
    std::string name;
    DWORD type;
    std::vector<uint8_t> data;
  };

  static std::vector<RegValue> enumValues(HKEY root, const std::string &path) {
    std::vector<RegValue> result;

    HKEY key;
    if (RegOpenKeyExA(root, path.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) {
      return result;
    }

    char name[256];
    BYTE data[4096];
    DWORD index = 0;

    while (true) {
      DWORD nameLen = sizeof(name);
      DWORD dataLen = sizeof(data);
      DWORD type;

      LONG err = RegEnumValueA(key, index, name, &nameLen, nullptr, &type, data,
                               &dataLen);
      if (err != ERROR_SUCCESS)
        break;

      RegValue val;
      val.name = std::string(name, nameLen);
      val.type = type;
      val.data.assign(data, data + dataLen);
      result.push_back(std::move(val));

      index++;
    }

    RegCloseKey(key);
    return result;
  }

  // Delete key
  static bool deleteKey(HKEY root, const std::string &path) {
    return RegDeleteKeyA(root, path.c_str()) == ERROR_SUCCESS;
  }

  // Delete value
  static bool deleteValue(HKEY root, const std::string &path,
                          const std::string &name) {
    HKEY key;
    if (RegOpenKeyExA(root, path.c_str(), 0, KEY_WRITE, &key) !=
        ERROR_SUCCESS) {
      return false;
    }
    LONG result = RegDeleteValueA(key, name.c_str());
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
  }
};

// =============================================================================
// PROCESS INFO
// =============================================================================
struct ProcessInfo {
  DWORD pid;
  DWORD parentPid;
  std::string name;
  std::string path;
};

// =============================================================================
// ADVANCED PROCESS MANAGEMENT
// =============================================================================
class ProcessAdv {
public:
  // List all processes using GetProcessImageFileName (no tlhelp32.h needed)
  static std::vector<ProcessInfo> list() {
    std::vector<ProcessInfo> result;

    // Enumerate by trying PIDs 0-65535 (avoids psapi header)
    for (DWORD pid = 4; pid < 65536; pid += 4) {
      HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
      if (h) {
        ProcessInfo info;
        info.pid = pid;
        info.parentPid = 0; // Would need NtQueryInformationProcess

        char path[MAX_PATH] = {0};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameA(h, 0, path, &size)) {
          info.path = path;
          // Extract name from path
          const char *name = strrchr(path, '\\');
          info.name = name ? (name + 1) : path;
        }

        CloseHandle(h);
        result.push_back(std::move(info));
      }
    }

    return result;
  }

  // Find process by name
  static std::vector<DWORD> findByName(const std::string &name) {
    std::vector<DWORD> result;
    auto procs = list();
    for (const auto &p : procs) {
      if (_stricmp(p.name.c_str(), name.c_str()) == 0) {
        result.push_back(p.pid);
      }
    }
    return result;
  }

  // Kill process
  static bool kill(DWORD pid, int exitCode = 0) {
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!h)
      return false;
    BOOL result = TerminateProcess(h, static_cast<UINT>(exitCode));
    CloseHandle(h);
    return result != 0;
  }

  // Spawn child process
  struct SpawnResult {
    bool success;
    DWORD pid;
    HANDLE hProcess;
    HANDLE hThread;
    std::string error;
  };

  static SpawnResult spawn(const std::string &command,
                           const std::string &workDir = "",
                           bool hidden = false) {
    SpawnResult result = {false, 0, nullptr, nullptr, ""};

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    if (hidden) {
      si.dwFlags = STARTF_USESHOWWINDOW;
      si.wShowWindow = SW_HIDE;
    }

    PROCESS_INFORMATION pi = {};

    BOOL ok =
        CreateProcessA(nullptr, const_cast<char *>(command.c_str()), nullptr,
                       nullptr, FALSE, hidden ? CREATE_NO_WINDOW : 0, nullptr,
                       workDir.empty() ? nullptr : workDir.c_str(), &si, &pi);

    if (!ok) {
      result.error = "CreateProcess failed: " + std::to_string(GetLastError());
      return result;
    }

    result.success = true;
    result.pid = pi.dwProcessId;
    result.hProcess = pi.hProcess;
    result.hThread = pi.hThread;

    return result;
  }

  // Wait for process
  static int wait(HANDLE hProcess, DWORD timeoutMs = INFINITE) {
    DWORD waitResult = WaitForSingleObject(hProcess, timeoutMs);
    if (waitResult == WAIT_TIMEOUT) {
      return -1; // Still running
    }

    DWORD exitCode = 0;
    GetExitCodeProcess(hProcess, &exitCode);
    return static_cast<int>(exitCode);
  }

  // Spawn and wait
  static int run(const std::string &command, DWORD timeoutMs = INFINITE,
                 const std::string &workDir = "") {
    auto result = spawn(command, workDir, true);
    if (!result.success)
      return -1;

    int exitCode = wait(result.hProcess, timeoutMs);

    CloseHandle(result.hThread);
    CloseHandle(result.hProcess);

    return exitCode;
  }

  // Read process output (spawn with pipes)
  static std::pair<int, std::string> runCapture(const std::string &command,
                                                DWORD timeoutMs = 30000) {
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
      return {-1, ""};
    }

    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};

    BOOL ok = CreateProcessA(nullptr, const_cast<char *>(command.c_str()),
                             nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                             nullptr, &si, &pi);

    CloseHandle(hWritePipe);

    if (!ok) {
      CloseHandle(hReadPipe);
      return {-1, ""};
    }

    // Read output
    std::string output;
    char buffer[4096];
    DWORD bytesRead;

    while (
        ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) &&
        bytesRead > 0) {
      buffer[bytesRead] = '\0';
      output += buffer;
    }

    CloseHandle(hReadPipe);

    WaitForSingleObject(pi.hProcess, timeoutMs);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return {static_cast<int>(exitCode), output};
  }
};

} // namespace Hub::System

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
// Registry binary
__declspec(dllexport) int hub_reg_get_binary(int root, const char *path,
                                             const char *name,
                                             unsigned char *buffer,
                                             int buffer_size);
__declspec(dllexport) int hub_reg_set_binary(int root, const char *path,
                                             const char *name,
                                             const unsigned char *data,
                                             int len);

// Registry enumeration
__declspec(dllexport) int hub_reg_enum_keys(int root, const char *path,
                                            char *buffer, int size);
__declspec(dllexport) int hub_reg_enum_values(int root, const char *path,
                                              char *buffer, int size);
__declspec(dllexport) int hub_reg_delete_key(int root, const char *path);
__declspec(dllexport) int hub_reg_delete_value(int root, const char *path,
                                               const char *name);

// Process management
__declspec(dllexport) int hub_proc_list(char *buffer, int size);
__declspec(dllexport) int hub_proc_find(const char *name, unsigned int *pids,
                                        int max_pids);
__declspec(dllexport) int hub_proc_kill(unsigned int pid);
__declspec(dllexport) unsigned int
hub_proc_spawn(const char *command, const char *work_dir, int hidden);
__declspec(dllexport) int hub_proc_wait(unsigned int pid, int timeout_ms);
__declspec(dllexport) int hub_proc_run(const char *command, int timeout_ms);
__declspec(dllexport) int hub_proc_run_capture(const char *command,
                                               char *output, int output_size);
}
