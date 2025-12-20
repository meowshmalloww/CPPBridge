#pragma once
// =============================================================================
// SANDBOX.H - Path Sandboxing & Security Validation
// =============================================================================
// Prevents:
// - Path traversal attacks (../)
// - Access outside allowed directories
// - Malicious path injection
// =============================================================================

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace Hub::Security {

namespace fs = std::filesystem;

// =============================================================================
// PATH VALIDATOR
// =============================================================================
class PathValidator {
public:
  static PathValidator &instance() {
    static PathValidator inst;
    return inst;
  }

  // Add allowed directory (sandbox root)
  void addAllowedPath(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
      auto canonical = fs::canonical(fs::path(path));
      allowed_paths_.insert(canonical.string());
    } catch (...) {
      // Path doesn't exist yet, store as-is
      allowed_paths_.insert(fs::path(path).lexically_normal().string());
    }
  }

  // Remove allowed directory
  void removeAllowedPath(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
      auto canonical = fs::canonical(fs::path(path));
      allowed_paths_.erase(canonical.string());
    } catch (...) {
      allowed_paths_.erase(fs::path(path).lexically_normal().string());
    }
  }

  // Clear all allowed paths
  void clearAllowedPaths() {
    std::lock_guard<std::mutex> lock(mutex_);
    allowed_paths_.clear();
  }

  // Check if path is safe (no traversal, within sandbox)
  bool isPathSafe(const std::string &path) const {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check for obvious traversal patterns
    if (containsTraversal(path))
      return false;

    // If no sandbox defined, all paths are allowed
    if (allowed_paths_.empty())
      return true;

    // Normalize the path
    fs::path normalized;
    try {
      if (fs::exists(path)) {
        normalized = fs::canonical(path);
      } else {
        normalized = fs::weakly_canonical(path);
      }
    } catch (...) {
      return false;
    }

    std::string normStr = normalized.string();

    // Check if path is under any allowed directory
    for (const auto &allowed : allowed_paths_) {
      if (isSubPath(normStr, allowed))
        return true;
    }

    return false;
  }

  // Validate and sanitize a path
  std::string sanitizePath(const std::string &path) const {
    fs::path p(path);

    // Remove redundant separators and normalize
    p = p.lexically_normal();

    // Convert to string
    std::string result = p.string();

    // Replace any remaining traversal attempts
    size_t pos;
    while ((pos = result.find("..")) != std::string::npos) {
      result.replace(pos, 2, "_");
    }

    return result;
  }

  // Get validation error message
  std::string getLastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_error_;
  }

  // Validate path and return detailed result
  struct ValidationResult {
    bool valid = false;
    std::string error;
    std::string normalized_path;
  };

  ValidationResult validate(const std::string &path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    ValidationResult result;

    // Check for empty path
    if (path.empty()) {
      result.error = "Path is empty";
      return result;
    }

    // Check for null bytes
    if (path.find('\0') != std::string::npos) {
      result.error = "Path contains null byte";
      return result;
    }

    // Check for traversal
    if (containsTraversal(path)) {
      result.error = "Path contains traversal sequence";
      return result;
    }

    // Normalize path
    try {
      fs::path p(path);
      result.normalized_path = p.lexically_normal().string();
    } catch (...) {
      result.error = "Invalid path format";
      return result;
    }

    // Check sandbox
    if (!allowed_paths_.empty()) {
      bool inSandbox = false;
      fs::path normalized;
      try {
        normalized = fs::weakly_canonical(path);
      } catch (...) {
        result.error = "Cannot resolve path";
        return result;
      }

      for (const auto &allowed : allowed_paths_) {
        if (isSubPath(normalized.string(), allowed)) {
          inSandbox = true;
          break;
        }
      }

      if (!inSandbox) {
        result.error = "Path outside sandbox";
        return result;
      }
    }

    result.valid = true;
    return result;
  }

private:
  PathValidator() = default;

  static bool containsTraversal(const std::string &path) {
    // Check for .. in various forms
    if (path.find("..\\") != std::string::npos)
      return true;
    if (path.find("../") != std::string::npos)
      return true;
    if (path.find("\\..") != std::string::npos)
      return true;
    if (path.find("/..") != std::string::npos)
      return true;

    // Check if path ends with ..
    if (path.size() >= 2 && path.substr(path.size() - 2) == "..")
      return true;

    // Check for encoded traversal
    if (path.find("%2e%2e") != std::string::npos)
      return true;
    if (path.find("%2E%2E") != std::string::npos)
      return true;

    return false;
  }

  static bool isSubPath(const std::string &path, const std::string &base) {
    // Normalize both paths for comparison
    std::string normPath = path;
    std::string normBase = base;

    // Convert to lowercase on Windows
    std::transform(normPath.begin(), normPath.end(), normPath.begin(),
                   ::tolower);
    std::transform(normBase.begin(), normBase.end(), normBase.begin(),
                   ::tolower);

    // Normalize separators
    std::replace(normPath.begin(), normPath.end(), '/', '\\');
    std::replace(normBase.begin(), normBase.end(), '/', '\\');

    // Check if path starts with base
    if (normPath.size() < normBase.size())
      return false;
    return normPath.compare(0, normBase.size(), normBase) == 0;
  }

  mutable std::mutex mutex_;
  std::set<std::string> allowed_paths_;
  mutable std::string last_error_;
};

// =============================================================================
// INPUT VALIDATOR
// =============================================================================
class InputValidator {
public:
  // Validate string input
  static bool isValidString(const char *str, size_t maxLen = 65536) {
    if (!str)
      return false;

    size_t len = 0;
    while (str[len] != '\0' && len < maxLen)
      ++len;

    return len < maxLen;
  }

  // Validate buffer
  static bool isValidBuffer(const void *buf, size_t size) {
    if (!buf)
      return false;
    if (size == 0)
      return false;
    if (size > 1024 * 1024 * 100)
      return false; // 100MB max
    return true;
  }

  // Validate integer range
  static bool isValidRange(int value, int min, int max) {
    return value >= min && value <= max;
  }

  // Sanitize string (remove control characters)
  static std::string sanitizeString(const std::string &input) {
    std::string result;
    result.reserve(input.size());
    for (char c : input) {
      if (c >= 32 || c == '\n' || c == '\r' || c == '\t') {
        result += c;
      }
    }
    return result;
  }
};

} // namespace Hub::Security

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) void hub_sandbox_add_path(const char *path);
__declspec(dllexport) void hub_sandbox_remove_path(const char *path);
__declspec(dllexport) void hub_sandbox_clear();
__declspec(dllexport) int hub_sandbox_is_safe(const char *path);
__declspec(dllexport) int hub_sandbox_validate(const char *path,
                                               char *error_buf, int error_size);
__declspec(dllexport) int hub_input_validate_string(const char *str,
                                                    int max_len);
}
