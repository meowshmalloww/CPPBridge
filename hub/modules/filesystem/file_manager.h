#pragma once
// =============================================================================
// FILE_MANAGER.H - Enhanced File System Module
// =============================================================================
// File operations with backup, copy, atomic writes, and directory management.
// Uses Windows-native APIs for maximum compatibility.
// =============================================================================

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <shlwapi.h>
#include <windows.h>

#pragma comment(lib, "shlwapi.lib")
#endif

namespace Hub::FileSystem {

// =============================================================================
// SIMPLE RESULT TYPE (Self-contained)
// =============================================================================
template <typename T> struct Result {
  bool success = false;
  T value;
  std::string error;

  static Result<T> Ok(const T &val) {
    Result<T> r;
    r.success = true;
    r.value = val;
    return r;
  }

  static Result<T> Err(const std::string &msg) {
    Result<T> r;
    r.success = false;
    r.error = msg;
    return r;
  }

  T &unwrap() { return value; }
  const T &unwrap() const { return value; }
  bool is_ok() const { return success; }
  bool is_err() const { return !success; }
};

// Specialization for void
template <> struct Result<void> {
  bool success = false;
  std::string error;

  static Result<void> Ok() {
    Result<void> r;
    r.success = true;
    return r;
  }

  static Result<void> Err(const std::string &msg) {
    Result<void> r;
    r.success = false;
    r.error = msg;
    return r;
  }

  bool is_ok() const { return success; }
  bool is_err() const { return !success; }
};

// =============================================================================
// FILE INFO STRUCTURE
// =============================================================================
struct FileInfo {
  std::string path;
  std::string name;
  uint64_t size;
  bool is_directory;
  bool is_readonly;
};

// =============================================================================
// HELPER FUNCTIONS (Windows-native)
// =============================================================================
inline bool exists(const std::string &path) {
#ifdef _WIN32
  DWORD attr = GetFileAttributesA(path.c_str());
  return attr != INVALID_FILE_ATTRIBUTES;
#else
  std::ifstream f(path);
  return f.good();
#endif
}

inline bool is_directory(const std::string &path) {
#ifdef _WIN32
  DWORD attr = GetFileAttributesA(path.c_str());
  return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
#else
  return false;
#endif
}

inline bool create_directories(const std::string &path) {
#ifdef _WIN32
  // Create full path recursively
  std::string current;
  for (size_t i = 0; i < path.size(); ++i) {
    current += path[i];
    if (path[i] == '\\' || path[i] == '/' || i == path.size() - 1) {
      if (!exists(current)) {
        if (!CreateDirectoryA(current.c_str(), NULL)) {
          if (GetLastError() != ERROR_ALREADY_EXISTS) {
            return false;
          }
        }
      }
    }
  }
  return true;
#else
  return false;
#endif
}

inline std::string get_parent_path(const std::string &path) {
  size_t pos = path.find_last_of("\\/");
  if (pos == std::string::npos)
    return "";
  return path.substr(0, pos);
}

inline std::string get_filename(const std::string &path) {
  size_t pos = path.find_last_of("\\/");
  if (pos == std::string::npos)
    return path;
  return path.substr(pos + 1);
}

// =============================================================================
// FILE OPERATIONS
// =============================================================================
inline Result<std::string> read_file(const std::string &path) {
  try {
    if (!exists(path)) {
      return Result<std::string>::Err("File not found: " + path);
    }

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
      return Result<std::string>::Err("Cannot open file: " + path);
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return Result<std::string>::Ok(ss.str());

  } catch (const std::exception &e) {
    return Result<std::string>::Err(std::string("Exception: ") + e.what());
  }
}

inline Result<void> write_file(const std::string &path,
                               const std::string &content) {
  try {
    // Create parent directories if needed
    std::string parent = get_parent_path(path);
    if (!parent.empty() && !exists(parent)) {
      create_directories(parent);
    }

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
      return Result<void>::Err("Cannot create file: " + path);
    }

    file << content;
    file.close();

    if (file.fail()) {
      return Result<void>::Err("Write failed: " + path);
    }

    return Result<void>::Ok();

  } catch (const std::exception &e) {
    return Result<void>::Err(std::string("Exception: ") + e.what());
  }
}

inline Result<void> copy_file(const std::string &source,
                              const std::string &dest, bool overwrite = false) {
  try {
    if (!exists(source)) {
      return Result<void>::Err("Source not found: " + source);
    }

    if (exists(dest) && !overwrite) {
      return Result<void>::Err("Destination exists: " + dest);
    }

    // Create parent directories for destination
    std::string parent = get_parent_path(dest);
    if (!parent.empty() && !exists(parent)) {
      create_directories(parent);
    }

#ifdef _WIN32
    if (!CopyFileA(source.c_str(), dest.c_str(), !overwrite)) {
      return Result<void>::Err("Copy failed: " + source + " -> " + dest);
    }
#else
    // Fallback: manual copy
    std::ifstream src(source, std::ios::binary);
    std::ofstream dst(dest, std::ios::binary);
    dst << src.rdbuf();
#endif

    return Result<void>::Ok();

  } catch (const std::exception &e) {
    return Result<void>::Err(std::string("Exception: ") + e.what());
  }
}

inline Result<void> delete_file(const std::string &path) {
  try {
    if (!exists(path)) {
      return Result<void>::Ok(); // Already doesn't exist
    }

#ifdef _WIN32
    if (!DeleteFileA(path.c_str())) {
      return Result<void>::Err("Delete failed: " + path);
    }
#else
    if (std::remove(path.c_str()) != 0) {
      return Result<void>::Err("Delete failed: " + path);
    }
#endif

    return Result<void>::Ok();

  } catch (const std::exception &e) {
    return Result<void>::Err(std::string("Exception: ") + e.what());
  }
}

inline Result<std::string> backup_file(const std::string &path) {
  try {
    if (!exists(path)) {
      return Result<std::string>::Err("File not found: " + path);
    }

    // Generate backup filename with timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char timestamp[32];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S",
                  std::localtime(&time));

    std::string backup_path = path + ".backup_" + timestamp;

    auto result = copy_file(path, backup_path, true);
    if (result.is_err()) {
      return Result<std::string>::Err(result.error);
    }

    return Result<std::string>::Ok(backup_path);

  } catch (const std::exception &e) {
    return Result<std::string>::Err(std::string("Exception: ") + e.what());
  }
}

inline Result<void> create_directory(const std::string &path) {
  try {
    if (exists(path)) {
      if (is_directory(path)) {
        return Result<void>::Ok(); // Already exists
      }
      return Result<void>::Err("Path exists but is not a directory: " + path);
    }

    if (!create_directories(path)) {
      return Result<void>::Err("Cannot create directory: " + path);
    }

    return Result<void>::Ok();

  } catch (const std::exception &e) {
    return Result<void>::Err(std::string("Exception: ") + e.what());
  }
}

} // namespace Hub::FileSystem
