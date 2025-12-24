#pragma once
// =============================================================================
// USB.H - USB Device Access Module (Simplified)
// =============================================================================
// Enumerate USB devices using Windows Registry.
// Using simplified approach to avoid SDK header conflicts.
// =============================================================================

#ifdef _WIN32
#include <windows.h>
#endif

#include <sstream>
#include <string>
#include <vector>


namespace Hub::Hardware {

// =============================================================================
// USB DEVICE INFO
// =============================================================================

struct USBDeviceInfo {
  std::string device_id;
  std::string description;
  bool connected;
};

// List USB devices from registry (simpler, no SetupAPI)
inline std::vector<USBDeviceInfo> list_usb_devices() {
  std::vector<USBDeviceInfo> devices;

#ifdef _WIN32
  HKEY hKey;
  const char *subKey = "SYSTEM\\CurrentControlSet\\Enum\\USB";

  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) ==
      ERROR_SUCCESS) {
    char name[256];
    DWORD nameLen = 256;
    DWORD index = 0;

    while (RegEnumKeyExA(hKey, index++, name, &nameLen, NULL, NULL, NULL,
                         NULL) == ERROR_SUCCESS) {
      USBDeviceInfo info;
      info.device_id = name;
      info.description = name;
      info.connected = true;
      devices.push_back(info);
      nameLen = 256;
    }

    RegCloseKey(hKey);
  }
#endif

  return devices;
}

// Get USB devices as JSON
inline std::string list_usb_devices_json() {
  auto devices = list_usb_devices();
  std::ostringstream ss;
  ss << "[";

  for (size_t i = 0; i < devices.size(); ++i) {
    if (i > 0)
      ss << ",";

    auto escape = [](const std::string &s) {
      std::string result;
      for (char c : s) {
        if (c == '"')
          result += "\\\"";
        else if (c == '\\')
          result += "\\\\";
        else
          result += c;
      }
      return result;
    };

    ss << "{\"device_id\":\"" << escape(devices[i].device_id) << "\""
       << ",\"description\":\"" << escape(devices[i].description) << "\""
       << ",\"connected\":" << (devices[i].connected ? "true" : "false") << "}";
  }

  ss << "]";
  return ss.str();
}

// Get USB device count
inline int get_usb_device_count() {
  return static_cast<int>(list_usb_devices().size());
}

} // namespace Hub::Hardware
