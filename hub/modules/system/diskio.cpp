// =============================================================================
// DISKIO.CPP - Disk I/O Monitoring C API
// =============================================================================

#include "diskio.h"

static thread_local std::string tl_drives_json;

extern "C" {

__declspec(dllexport) uint64_t hub_disk_get_total(const char *drive) {
  return Hub::System::get_disk_total(drive ? drive : "C");
}

__declspec(dllexport) uint64_t hub_disk_get_free(const char *drive) {
  return Hub::System::get_disk_free(drive ? drive : "C");
}

__declspec(dllexport) int hub_disk_get_used_percent(const char *drive) {
  return Hub::System::get_disk_used_percent(drive ? drive : "C");
}

__declspec(dllexport) const char *hub_disk_list_drives() {
  tl_drives_json = Hub::System::list_drives_json();
  return tl_drives_json.c_str();
}

} // extern "C"
