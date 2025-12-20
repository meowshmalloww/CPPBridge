// =============================================================================
// SYSUTILS.CPP - System Utilities Implementation
// =============================================================================

#include "sysutils.h"

static thread_local std::string tl_env_value;
static thread_local std::string tl_env_expanded;
static thread_local std::string tl_process_path;

extern "C" {

__declspec(dllexport) const char *hub_env_get(const char *name) {
  if (!name)
    return "";
  auto val = Hub::System::Environment::get(name);
  tl_env_value = val.value_or("");
  return tl_env_value.c_str();
}

__declspec(dllexport) int hub_env_set(const char *name, const char *value) {
  if (!name || !value)
    return 0;
  return Hub::System::Environment::set(name, value) ? 1 : 0;
}

__declspec(dllexport) const char *hub_env_expand(const char *str) {
  if (!str)
    return "";
  tl_env_expanded = Hub::System::Environment::expand(str);
  return tl_env_expanded.c_str();
}

__declspec(dllexport) const char *hub_env_home() {
  static std::string home = Hub::System::Environment::userHome();
  return home.c_str();
}

__declspec(dllexport) const char *hub_env_temp() {
  static std::string temp = Hub::System::Environment::tempDir();
  return temp.c_str();
}

__declspec(dllexport) const char *hub_env_appdata() {
  static std::string appdata = Hub::System::Environment::appData();
  return appdata.c_str();
}

__declspec(dllexport) unsigned int hub_process_id() {
  return Hub::System::Process::currentId();
}

__declspec(dllexport) const char *hub_process_path() {
  tl_process_path = Hub::System::Process::currentPath();
  return tl_process_path.c_str();
}

__declspec(dllexport) int hub_process_exists(unsigned int pid) {
  return Hub::System::Process::exists(pid) ? 1 : 0;
}

__declspec(dllexport) unsigned int hub_cpu_count() {
  return Hub::System::Process::cpuCount();
}

} // extern "C"
