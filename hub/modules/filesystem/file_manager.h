#pragma once
// =============================================================================
// FILE_MANAGER.H - Enhanced File System Module
// =============================================================================
// File operations with backup, copy, atomic writes, and directory management.
// Self-contained module with minimal dependencies.
// =============================================================================

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


namespace fs = std::filesystem;

namespace Hub::FileSystem {

// =============================================================================
// SIMPLE RESULT TYPE (Self-contained)
// =============================================================================
template <typename T> struct Result {
  bool success = false;
  T value;
  std::string error;

  static Result Ok(const T &val) {
    Result r;
    r.success = true;
    r.value = val;
    return r;
  }

  static Result Ok(T &&val) {
    Result r;
    r.success = true;
    r.value = std::move(val);
    return r;
  }

  static Result Err(const std::string &msg) {
    Result r;
    r.success = false;
    r.error = msg;
    return r;
  }

  bool is_ok() const { return success; }
  bool is_err() const { return !success; }
  const T &unwrap() const { return value; }
};

template <> struct Result<void> {
  bool success = false;
  std::string error;

  static Result Ok() {
    Result r;
    r.success = true;
    return r;
  }

  static Result Err(const std::string &msg) {
    Result r;
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
// FILE OPERATIONS
// =============================================================================
inline Result<std::string> read_file(const std::string &path) {
  try {
    if (!fs::exists(path)) {
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

inline Result<std::vector<uint8_t>> read_file_binary(const std::string &path) {
  try {
    if (!fs::exists(path)) {
      return Result<std::vector<uint8_t>>::Err("File not found: " + path);
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      return Result<std::vector<uint8_t>>::Err("Cannot open file: " + path);
    }

    auto size = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    file.read(reinterpret_cast<char *>(buffer.data()), size);

    return Result<std::vector<uint8_t>>::Ok(std::move(buffer));

  } catch (const std::exception &e) {
    return Result<std::vector<uint8_t>>::Err(std::string("Exception: ") +
                                             e.what());
  }
}

inline Result<void> write_file(const std::string &path,
                               const std::string &content) {
  try {
    fs::path filepath(path);
    if (filepath.has_parent_path()) {
      fs::create_directories(filepath.parent_path());
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
      return Result<void>::Err("Cannot create file: " + path);
    }

    file.write(content.data(), content.size());
    file.close();

    std::cout << "[filesystem] Wrote " << content.size() << " bytes to " << path
              << "\n";
    return Result<void>::Ok();

  } catch (const std::exception &e) {
    return Result<void>::Err(std::string("Exception: ") + e.what());
  }
}

inline Result<void> write_file_binary(const std::string &path,
                                      const std::vector<uint8_t> &data) {
  try {
    fs::path filepath(path);
    if (filepath.has_parent_path()) {
      fs::create_directories(filepath.parent_path());
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
      return Result<void>::Err("Cannot create file: " + path);
    }

    file.write(reinterpret_cast<const char *>(data.data()), data.size());
    file.close();

    return Result<void>::Ok();

  } catch (const std::exception &e) {
    return Result<void>::Err(std::string("Exception: ") + e.what());
  }
}

inline Result<void> copy_file(const std::string &source,
                              const std::string &dest, bool overwrite = false) {
  try {
    if (!fs::exists(source)) {
      return Result<void>::Err("Source not found: " + source);
    }

    if (fs::exists(dest) && !overwrite) {
      return Result<void>::Err("Destination exists: " + dest);
    }

    fs::path destPath(dest);
    if (destPath.has_parent_path()) {
      fs::create_directories(destPath.parent_path());
    }

    fs::copy_options opts = overwrite ? fs::copy_options::overwrite_existing
                                      : fs::copy_options::none;
    fs::copy_file(source, dest, opts);

    std::cout << "[filesystem] Copied: " << source << " -> " << dest << "\n";
    return Result<void>::Ok();

  } catch (const std::exception &e) {
    return Result<void>::Err(std::string("Copy failed: ") + e.what());
  }
}

inline Result<void> move_file(const std::string &source,
                              const std::string &dest) {
  try {
    if (!fs::exists(source)) {
      return Result<void>::Err("Source not found: " + source);
    }

    fs::path destPath(dest);
    if (destPath.has_parent_path()) {
      fs::create_directories(destPath.parent_path());
    }

    fs::rename(source, dest);
    return Result<void>::Ok();

  } catch (const std::exception &e) {
    return Result<void>::Err(std::string("Move failed: ") + e.what());
  }
}

inline Result<void> delete_file(const std::string &path) {
  try {
    if (!fs::exists(path)) {
      return Result<void>::Ok();
    }

    fs::remove(path);
    return Result<void>::Ok();

  } catch (const std::exception &e) {
    return Result<void>::Err(std::string("Delete failed: ") + e.what());
  }
}

inline Result<std::string> backup_file(const std::string &path) {
  if (!fs::exists(path)) {
    return Result<std::string>::Err("File not found: " + path);
  }

  auto now = std::chrono::system_clock::now();
  auto time_t_val = std::chrono::system_clock::to_time_t(now);
  char timestamp[32];
  struct tm timeinfo;
#ifdef _WIN32
  localtime_s(&timeinfo, &time_t_val);
#else
  localtime_r(&time_t_val, &timeinfo);
#endif
  std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &timeinfo);

  fs::path original(path);
  std::string backup_path = original.parent_path().string() + "/" +
                            original.stem().string() + "_backup_" + timestamp +
                            original.extension().string();

  auto result = copy_file(path, backup_path, false);
  if (result.is_err()) {
    return Result<std::string>::Err(result.error);
  }

  std::cout << "[filesystem] Backup created: " << backup_path << "\n";
  return Result<std::string>::Ok(backup_path);
}

inline Result<void> create_directory(const std::string &path) {
  try {
    fs::create_directories(path);
    return Result<void>::Ok();
  } catch (const std::exception &e) {
    return Result<void>::Err(std::string("Create directory failed: ") +
                             e.what());
  }
}

inline Result<std::vector<FileInfo>> list_directory(const std::string &path) {
  try {
    if (!fs::exists(path)) {
      return Result<std::vector<FileInfo>>::Err("Directory not found: " + path);
    }

    std::vector<FileInfo> entries;
    for (const auto &entry : fs::directory_iterator(path)) {
      FileInfo info;
      info.path = entry.path().string();
      info.name = entry.path().filename().string();
      info.is_directory = entry.is_directory();
      info.size = entry.is_regular_file() ? entry.file_size() : 0;
      entries.push_back(info);
    }

    return Result<std::vector<FileInfo>>::Ok(std::move(entries));

  } catch (const std::exception &e) {
    return Result<std::vector<FileInfo>>::Err(std::string("List failed: ") +
                                              e.what());
  }
}

inline bool exists(const std::string &path) { return fs::exists(path); }
inline bool is_directory(const std::string &path) {
  return fs::is_directory(path);
}

inline Result<uint64_t> file_size(const std::string &path) {
  try {
    if (!fs::exists(path)) {
      return Result<uint64_t>::Err("File not found: " + path);
    }
    return Result<uint64_t>::Ok(fs::file_size(path));
  } catch (const std::exception &e) {
    return Result<uint64_t>::Err(std::string("Size check failed: ") + e.what());
  }
}

} // namespace Hub::FileSystem

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_file_read(const char *path, char *buffer,
                                        int buffer_size);
__declspec(dllexport) int hub_file_write(const char *path, const char *content);
__declspec(dllexport) int hub_file_copy(const char *source, const char *dest,
                                        int overwrite);
__declspec(dllexport) int hub_file_delete(const char *path);
__declspec(dllexport) int hub_file_exists(const char *path);
__declspec(dllexport) int hub_file_backup(const char *path, char *backup_path,
                                          int buffer_size);
__declspec(dllexport) int hub_dir_create(const char *path);
__declspec(dllexport) const char *hub_file_get_error();
}
