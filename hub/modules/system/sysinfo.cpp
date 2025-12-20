// =============================================================================
// SYSINFO.CPP - System Information Implementation
// =============================================================================

#include "sysinfo.h"

static thread_local std::string tl_vendor;
static thread_local std::string tl_brand;
static thread_local std::string tl_os_name;
static thread_local std::string tl_computer;
static thread_local std::string tl_user;
static thread_local std::string tl_fingerprint;

extern "C" {

__declspec(dllexport) int hub_sys_cpu_cores() {
  return Hub::System::get_cpu_info().cores;
}

__declspec(dllexport) int hub_sys_cpu_threads() {
  return Hub::System::get_cpu_info().threads;
}

__declspec(dllexport) const char *hub_sys_cpu_vendor() {
  tl_vendor = Hub::System::get_cpu_info().vendor;
  return tl_vendor.c_str();
}

__declspec(dllexport) const char *hub_sys_cpu_brand() {
  tl_brand = Hub::System::get_cpu_info().brand;
  return tl_brand.c_str();
}

__declspec(dllexport) uint64_t hub_sys_mem_total() {
  return Hub::System::get_memory_info().total_physical;
}

__declspec(dllexport) uint64_t hub_sys_mem_available() {
  return Hub::System::get_memory_info().available_physical;
}

__declspec(dllexport) int hub_sys_mem_load_percent() {
  return Hub::System::get_memory_info().memory_load_percent;
}

__declspec(dllexport) const char *hub_sys_os_name() {
  tl_os_name = Hub::System::get_os_info().name;
  return tl_os_name.c_str();
}

__declspec(dllexport) const char *hub_sys_computer_name() {
  tl_computer = Hub::System::get_os_info().computer_name;
  return tl_computer.c_str();
}

__declspec(dllexport) const char *hub_sys_user_name() {
  tl_user = Hub::System::get_os_info().user_name;
  return tl_user.c_str();
}

__declspec(dllexport) const char *hub_sys_fingerprint() {
  tl_fingerprint = Hub::System::generate_fingerprint();
  return tl_fingerprint.c_str();
}

__declspec(dllexport) uint64_t hub_sys_uptime_ms() {
  return Hub::System::get_system_uptime_ms();
}

} // extern "C"
