#pragma once
// =============================================================================
// VERSION.H - Version Compatibility Module
// =============================================================================
// Version information and compatibility checking.
// Allows apps to check DLL version and ensure compatibility.
// =============================================================================

#include <sstream>
#include <string>


namespace Hub::Core {

// =============================================================================
// VERSION CONSTANTS
// =============================================================================

constexpr int VERSION_MAJOR = 5;
constexpr int VERSION_MINOR = 3;
constexpr int VERSION_PATCH = 0;

// =============================================================================
// VERSION FUNCTIONS
// =============================================================================

inline std::string get_version() {
  std::ostringstream ss;
  ss << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH;
  return ss.str();
}

inline int get_version_major() { return VERSION_MAJOR; }

inline int get_version_minor() { return VERSION_MINOR; }

inline int get_version_patch() { return VERSION_PATCH; }

// Get version as single integer for easy comparison
// Format: MAJOR * 10000 + MINOR * 100 + PATCH
inline int get_version_number() {
  return VERSION_MAJOR * 10000 + VERSION_MINOR * 100 + VERSION_PATCH;
}

// =============================================================================
// COMPATIBILITY CHECKING
// =============================================================================

// Parse version string "X.Y.Z" to version number
inline int parse_version(const std::string &version) {
  int major = 0, minor = 0, patch = 0;

  std::istringstream ss(version);
  char dot;
  ss >> major >> dot >> minor >> dot >> patch;

  return major * 10000 + minor * 100 + patch;
}

// Check if current version is compatible with minimum required version
// Compatible if current >= minimum
inline bool check_compat(const std::string &min_version) {
  int current = get_version_number();
  int minimum = parse_version(min_version);
  return current >= minimum;
}

// Check if current version is compatible with a version range
// Compatible if min <= current <= max
inline bool check_compat_range(const std::string &min_version,
                               const std::string &max_version) {
  int current = get_version_number();
  int minimum = parse_version(min_version);
  int maximum = parse_version(max_version);
  return current >= minimum && current <= maximum;
}

// Allow any version (always returns true)
// Use this when you want maximum compatibility
inline bool allow_any_version() { return true; }

// Get version info as JSON
inline std::string get_version_info_json() {
  std::ostringstream ss;
  ss << "{\"version\":\"" << get_version() << "\""
     << ",\"major\":" << VERSION_MAJOR << ",\"minor\":" << VERSION_MINOR
     << ",\"patch\":" << VERSION_PATCH << ",\"number\":" << get_version_number()
     << "}";
  return ss.str();
}

} // namespace Hub::Core
