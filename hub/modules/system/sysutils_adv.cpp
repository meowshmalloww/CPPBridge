// =============================================================================
// SYSUTILS_ADV.CPP - Advanced System Utilities Implementation
// =============================================================================

#include "sysutils_adv.h"
#include <cstring>
#include <map>
#include <sstream>


static HKEY getRootKey(int root) {
  switch (root) {
  case 0:
    return HKEY_CLASSES_ROOT;
  case 1:
    return HKEY_CURRENT_USER;
  case 2:
    return HKEY_LOCAL_MACHINE;
  case 3:
    return HKEY_USERS;
  default:
    return HKEY_CURRENT_USER;
  }
}

// Store spawned processes
static std::map<DWORD, HANDLE> g_processes;

extern "C" {

__declspec(dllexport) int hub_reg_get_binary(int root, const char *path,
                                             const char *name,
                                             unsigned char *buffer,
                                             int buffer_size) {
  if (!path || !name || !buffer || buffer_size <= 0)
    return 0;

  try {
    auto data =
        Hub::System::RegistryAdv::getBinary(getRootKey(root), path, name);
    if (!data)
      return 0;

    int len = static_cast<int>(data->size());
    if (len > buffer_size)
      len = buffer_size;
    std::memcpy(buffer, data->data(), len);
    return len;
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) int hub_reg_set_binary(int root, const char *path,
                                             const char *name,
                                             const unsigned char *data,
                                             int len) {
  if (!path || !name || !data || len <= 0)
    return 0;
  try {
    return Hub::System::RegistryAdv::setBinary(getRootKey(root), path, name,
                                               data, len)
               ? 1
               : 0;
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) int hub_reg_enum_keys(int root, const char *path,
                                            char *buffer, int size) {
  if (!path || !buffer || size <= 0)
    return 0;

  try {
    auto keys = Hub::System::RegistryAdv::enumKeys(getRootKey(root), path);

    std::ostringstream oss;
    for (size_t i = 0; i < keys.size(); ++i) {
      if (i > 0)
        oss << '\n';
      oss << keys[i];
    }

    std::string result = oss.str();
    int len = static_cast<int>(result.size());
    if (len >= size)
      len = size - 1;
    std::memcpy(buffer, result.c_str(), len);
    buffer[len] = '\0';
    return static_cast<int>(keys.size());
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) int hub_reg_enum_values(int root, const char *path,
                                              char *buffer, int size) {
  if (!path || !buffer || size <= 0)
    return 0;

  try {
    auto values = Hub::System::RegistryAdv::enumValues(getRootKey(root), path);

    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
      if (i > 0)
        oss << '\n';
      oss << values[i].name << '=' << values[i].type;
    }

    std::string result = oss.str();
    int len = static_cast<int>(result.size());
    if (len >= size)
      len = size - 1;
    std::memcpy(buffer, result.c_str(), len);
    buffer[len] = '\0';
    return static_cast<int>(values.size());
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) int hub_reg_delete_key(int root, const char *path) {
  if (!path)
    return 0;
  try {
    return Hub::System::RegistryAdv::deleteKey(getRootKey(root), path) ? 1 : 0;
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) int hub_reg_delete_value(int root, const char *path,
                                               const char *name) {
  if (!path || !name)
    return 0;
  try {
    return Hub::System::RegistryAdv::deleteValue(getRootKey(root), path, name)
               ? 1
               : 0;
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) int hub_proc_list(char *buffer, int size) {
  if (!buffer || size <= 0)
    return 0;

  try {
    auto procs = Hub::System::ProcessAdv::list();

    std::ostringstream oss;
    for (size_t i = 0; i < procs.size(); ++i) {
      if (i > 0)
        oss << '\n';
      oss << procs[i].pid << ':' << procs[i].name;
    }

    std::string result = oss.str();
    int len = static_cast<int>(result.size());
    if (len >= size)
      len = size - 1;
    std::memcpy(buffer, result.c_str(), len);
    buffer[len] = '\0';
    return static_cast<int>(procs.size());
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) int hub_proc_find(const char *name, unsigned int *pids,
                                        int max_pids) {
  if (!name || !pids || max_pids <= 0)
    return 0;

  try {
    auto found = Hub::System::ProcessAdv::findByName(name);
    int count = static_cast<int>(found.size());
    if (count > max_pids)
      count = max_pids;
    for (int i = 0; i < count; ++i) {
      pids[i] = found[i];
    }
    return count;
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) int hub_proc_kill(unsigned int pid) {
  try {
    return Hub::System::ProcessAdv::kill(pid) ? 1 : 0;
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) unsigned int
hub_proc_spawn(const char *command, const char *work_dir, int hidden) {
  if (!command)
    return 0;

  try {
    auto result = Hub::System::ProcessAdv::spawn(
        command, work_dir ? work_dir : "", hidden != 0);

    if (result.success) {
      g_processes[result.pid] = result.hProcess;
      CloseHandle(result.hThread);
      return result.pid;
    }
    return 0;
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) int hub_proc_wait(unsigned int pid, int timeout_ms) {
  try {
    auto it = g_processes.find(pid);
    if (it == g_processes.end())
      return -1;

    int exitCode = Hub::System::ProcessAdv::wait(
        it->second, timeout_ms < 0 ? INFINITE : static_cast<DWORD>(timeout_ms));

    if (exitCode != -1) {
      CloseHandle(it->second);
      g_processes.erase(it);
    }

    return exitCode;
  } catch (...) {
    return -1;
  }
}

__declspec(dllexport) int hub_proc_run(const char *command, int timeout_ms) {
  if (!command)
    return -1;

  try {
    return Hub::System::ProcessAdv::run(
        command, timeout_ms < 0 ? INFINITE : static_cast<DWORD>(timeout_ms));
  } catch (...) {
    return -1;
  }
}

__declspec(dllexport) int hub_proc_run_capture(const char *command,
                                               char *output, int output_size) {
  if (!command || !output || output_size <= 0)
    return -1;

  try {
    auto [exitCode, out] = Hub::System::ProcessAdv::runCapture(command);

    int len = static_cast<int>(out.size());
    if (len >= output_size)
      len = output_size - 1;
    std::memcpy(output, out.c_str(), len);
    output[len] = '\0';

    return exitCode;
  } catch (...) {
    return -1;
  }
}

} // extern "C"
