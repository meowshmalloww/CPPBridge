#pragma once
// =============================================================================
// DISKIO.H - Disk I/O Monitoring Module
// =============================================================================
// Disk space info, drive listing, and storage statistics.
// Uses Windows GetDiskFreeSpaceEx and GetLogicalDrives.
// =============================================================================

#ifdef _WIN32
#include <windows.h>
#endif

#include <sstream>
#include <string>
#include <vector>


namespace Hub::System {

// =============================================================================
// DISK SPACE INFO
// =============================================================================

struct DiskSpaceInfo {
  uint64_t total_bytes;
  uint64_t free_bytes;
  uint64_t used_bytes;
  int used_percent;
  bool valid;
};

inline DiskSpaceInfo get_disk_space(const std::string &drive) {
  DiskSpaceInfo info = {0, 0, 0, 0, false};

#ifdef _WIN32
  ULARGE_INTEGER freeBytesAvailable, totalBytes, freeBytes;

  std::string path = drive;
  if (path.empty())
    path = "C:\\";
  if (path.length() == 1)
    path += ":\\";

  if (GetDiskFreeSpaceExA(path.c_str(), &freeBytesAvailable, &totalBytes,
                          &freeBytes)) {
    info.total_bytes = totalBytes.QuadPart;
    info.free_bytes = freeBytes.QuadPart;
    info.used_bytes = totalBytes.QuadPart - freeBytes.QuadPart;
    if (totalBytes.QuadPart > 0) {
      info.used_percent =
          static_cast<int>((info.used_bytes * 100) / info.total_bytes);
    }
    info.valid = true;
  }
#else
  (void)drive;
#endif

  return info;
}

inline uint64_t get_disk_total(const std::string &drive) {
  return get_disk_space(drive).total_bytes;
}

inline uint64_t get_disk_free(const std::string &drive) {
  return get_disk_space(drive).free_bytes;
}

inline int get_disk_used_percent(const std::string &drive) {
  return get_disk_space(drive).used_percent;
}

// =============================================================================
// DRIVE LISTING
// =============================================================================

struct DriveInfo {
  std::string letter;
  std::string type;
  std::string label;
  uint64_t total_bytes;
  uint64_t free_bytes;
};

inline std::vector<DriveInfo> list_drives() {
  std::vector<DriveInfo> drives;

#ifdef _WIN32
  DWORD drivesMask = GetLogicalDrives();

  for (char letter = 'A'; letter <= 'Z'; ++letter) {
    if (drivesMask & (1 << (letter - 'A'))) {
      DriveInfo drive;
      drive.letter = std::string(1, letter);

      std::string path = std::string(1, letter) + ":\\";

      // Get drive type
      UINT driveType = GetDriveTypeA(path.c_str());
      switch (driveType) {
      case DRIVE_REMOVABLE:
        drive.type = "Removable";
        break;
      case DRIVE_FIXED:
        drive.type = "Fixed";
        break;
      case DRIVE_REMOTE:
        drive.type = "Network";
        break;
      case DRIVE_CDROM:
        drive.type = "CD-ROM";
        break;
      case DRIVE_RAMDISK:
        drive.type = "RAM Disk";
        break;
      default:
        drive.type = "Unknown";
        break;
      }

      // Get volume label
      char volumeName[256] = {0};
      if (GetVolumeInformationA(path.c_str(), volumeName, sizeof(volumeName),
                                NULL, NULL, NULL, NULL, 0)) {
        drive.label = volumeName;
      }

      // Get disk space
      auto space = get_disk_space(path);
      drive.total_bytes = space.total_bytes;
      drive.free_bytes = space.free_bytes;

      drives.push_back(drive);
    }
  }
#endif

  return drives;
}

// Get drives as JSON string
inline std::string list_drives_json() {
  auto drives = list_drives();
  std::ostringstream ss;
  ss << "[";

  for (size_t i = 0; i < drives.size(); ++i) {
    if (i > 0)
      ss << ",";
    ss << "{\"letter\":\"" << drives[i].letter << "\""
       << ",\"type\":\"" << drives[i].type << "\""
       << ",\"label\":\"" << drives[i].label << "\""
       << ",\"total\":" << drives[i].total_bytes
       << ",\"free\":" << drives[i].free_bytes << "}";
  }

  ss << "]";
  return ss.str();
}

} // namespace Hub::System
