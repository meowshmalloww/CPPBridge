#pragma once
// =============================================================================
// SYSINFO.H - System Information Module
// =============================================================================
// Hardware fingerprinting, CPU/Memory info, and system detection.
// =============================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <intrin.h>
#include <windows.h>


#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace Hub::System {

// =============================================================================
// CPU INFO
// =============================================================================
struct CpuInfo {
  std::string vendor;
  std::string brand;
  int cores = 0;
  int threads = 0;
  int family = 0;
  int model = 0;
};

inline CpuInfo get_cpu_info() {
  CpuInfo info;
  int cpuInfo[4] = {0};

  // Get vendor string
  __cpuid(cpuInfo, 0);
  char vendor[13] = {0};
  *reinterpret_cast<int *>(vendor) = cpuInfo[1];
  *reinterpret_cast<int *>(vendor + 4) = cpuInfo[3];
  *reinterpret_cast<int *>(vendor + 8) = cpuInfo[2];
  info.vendor = vendor;

  // Get brand string
  char brand[49] = {0};
  __cpuid(cpuInfo, 0x80000000);
  if (static_cast<unsigned>(cpuInfo[0]) >= 0x80000004) {
    __cpuid(reinterpret_cast<int *>(brand), 0x80000002);
    __cpuid(reinterpret_cast<int *>(brand + 16), 0x80000003);
    __cpuid(reinterpret_cast<int *>(brand + 32), 0x80000004);
    info.brand = brand;
    size_t start = info.brand.find_first_not_of(' ');
    if (start != std::string::npos) {
      info.brand = info.brand.substr(start);
    }
  }

  // Get family/model
  __cpuid(cpuInfo, 1);
  info.model = (cpuInfo[0] >> 4) & 0xF;
  info.family = (cpuInfo[0] >> 8) & 0xF;

  // Get thread count
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  info.threads = static_cast<int>(sysInfo.dwNumberOfProcessors);
  info.cores = info.threads; // Simplified

  return info;
}

// =============================================================================
// MEMORY INFO
// =============================================================================
struct MemoryInfo {
  uint64_t total_physical = 0;
  uint64_t available_physical = 0;
  int memory_load_percent = 0;
};

inline MemoryInfo get_memory_info() {
  MemoryInfo info;
  MEMORYSTATUSEX memStatus;
  memStatus.dwLength = sizeof(memStatus);

  if (GlobalMemoryStatusEx(&memStatus)) {
    info.total_physical = memStatus.ullTotalPhys;
    info.available_physical = memStatus.ullAvailPhys;
    info.memory_load_percent = static_cast<int>(memStatus.dwMemoryLoad);
  }

  return info;
}

// =============================================================================
// OS INFO
// =============================================================================
struct OsInfo {
  std::string name;
  std::string version;
  std::string architecture;
  std::string computer_name;
  std::string user_name;
};

inline OsInfo get_os_info() {
  OsInfo info;
  info.name = "Windows";
  info.version = "10.0";

  // Architecture
  SYSTEM_INFO sysInfo;
  GetNativeSystemInfo(&sysInfo);
  switch (sysInfo.wProcessorArchitecture) {
  case PROCESSOR_ARCHITECTURE_AMD64:
    info.architecture = "x64";
    break;
  case PROCESSOR_ARCHITECTURE_ARM64:
    info.architecture = "ARM64";
    break;
  case PROCESSOR_ARCHITECTURE_INTEL:
    info.architecture = "x86";
    break;
  default:
    info.architecture = "Unknown";
    break;
  }

  // Computer name
  char computerName[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD size = sizeof(computerName);
  if (GetComputerNameA(computerName, &size)) {
    info.computer_name = computerName;
  }

  // User name
  char userName[256];
  size = sizeof(userName);
  if (GetUserNameA(userName, &size)) {
    info.user_name = userName;
  }

  return info;
}

// =============================================================================
// HARDWARE FINGERPRINT
// =============================================================================
inline std::string generate_fingerprint() {
  std::ostringstream oss;
  auto cpu = get_cpu_info();
  auto mem = get_memory_info();
  auto os = get_os_info();

  oss << cpu.vendor << "|";
  oss << cpu.family << "-" << cpu.model << "|";
  oss << cpu.cores << "c" << cpu.threads << "t|";
  oss << (mem.total_physical / (1024 * 1024 * 1024)) << "GB|";
  oss << os.architecture << "|" << os.computer_name;

  return oss.str();
}

// =============================================================================
// UPTIME
// =============================================================================
inline uint64_t get_system_uptime_ms() { return GetTickCount64(); }

} // namespace Hub::System

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_sys_cpu_cores();
__declspec(dllexport) int hub_sys_cpu_threads();
__declspec(dllexport) const char *hub_sys_cpu_vendor();
__declspec(dllexport) const char *hub_sys_cpu_brand();
__declspec(dllexport) uint64_t hub_sys_mem_total();
__declspec(dllexport) uint64_t hub_sys_mem_available();
__declspec(dllexport) int hub_sys_mem_load_percent();
__declspec(dllexport) const char *hub_sys_os_name();
__declspec(dllexport) const char *hub_sys_computer_name();
__declspec(dllexport) const char *hub_sys_user_name();
__declspec(dllexport) const char *hub_sys_fingerprint();
__declspec(dllexport) uint64_t hub_sys_uptime_ms();
}
