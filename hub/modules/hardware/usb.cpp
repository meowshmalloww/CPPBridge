// =============================================================================
// USB.CPP - USB Device Access C API
// =============================================================================

#include "usb.h"

static thread_local std::string tl_usb_json;

extern "C" {

__declspec(dllexport) const char *hub_usb_list_devices() {
  tl_usb_json = Hub::Hardware::list_usb_devices_json();
  return tl_usb_json.c_str();
}

__declspec(dllexport) int hub_usb_get_device_count() {
  return Hub::Hardware::get_usb_device_count();
}

} // extern "C"
