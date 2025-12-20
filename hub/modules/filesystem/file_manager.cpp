// =============================================================================
// FILE_MANAGER.CPP - File Manager Implementation
// =============================================================================

#include "file_manager.h"
#include <cstring>

// Thread-local error storage
static thread_local std::string tl_last_error;
static thread_local std::string tl_backup_path;

extern "C" {

__declspec(dllexport) int hub_file_read(const char *path, char *buffer,
                                        int buffer_size) {
  if (!path) {
    tl_last_error = "Path is null";
    return -1;
  }

  auto result = Hub::FileSystem::read_file(path);
  if (result.is_err()) {
    tl_last_error = result.error;
    return -1;
  }

  const std::string &content = result.unwrap();
  if (buffer && buffer_size > 0) {
    size_t copy_size =
        (std::min)(static_cast<size_t>(buffer_size - 1), content.size());
    std::memcpy(buffer, content.data(), copy_size);
    buffer[copy_size] = '\0';
  }

  return static_cast<int>(content.size());
}

__declspec(dllexport) int hub_file_write(const char *path,
                                         const char *content) {
  if (!path || !content) {
    tl_last_error = "Path or content is null";
    return -1;
  }

  auto result = Hub::FileSystem::write_file(path, content);
  if (result.is_err()) {
    tl_last_error = result.error;
    return -1;
  }

  return 0;
}

__declspec(dllexport) int hub_file_copy(const char *source, const char *dest,
                                        int overwrite) {
  if (!source || !dest) {
    tl_last_error = "Source or dest is null";
    return -1;
  }

  auto result = Hub::FileSystem::copy_file(source, dest, overwrite != 0);
  if (result.is_err()) {
    tl_last_error = result.error;
    return -1;
  }

  return 0;
}

__declspec(dllexport) int hub_file_delete(const char *path) {
  if (!path) {
    tl_last_error = "Path is null";
    return -1;
  }

  auto result = Hub::FileSystem::delete_file(path);
  if (result.is_err()) {
    tl_last_error = result.error;
    return -1;
  }

  return 0;
}

__declspec(dllexport) int hub_file_exists(const char *path) {
  if (!path)
    return 0;
  return Hub::FileSystem::exists(path) ? 1 : 0;
}

__declspec(dllexport) int hub_file_backup(const char *path, char *backup_path,
                                          int buffer_size) {
  if (!path) {
    tl_last_error = "Path is null";
    return -1;
  }

  auto result = Hub::FileSystem::backup_file(path);
  if (result.is_err()) {
    tl_last_error = result.error;
    return -1;
  }

  tl_backup_path = result.unwrap();

  if (backup_path && buffer_size > 0) {
    size_t copy_size =
        (std::min)(static_cast<size_t>(buffer_size - 1), tl_backup_path.size());
    std::memcpy(backup_path, tl_backup_path.data(), copy_size);
    backup_path[copy_size] = '\0';
  }

  return 0;
}

__declspec(dllexport) int hub_dir_create(const char *path) {
  if (!path) {
    tl_last_error = "Path is null";
    return -1;
  }

  auto result = Hub::FileSystem::create_directory(path);
  if (result.is_err()) {
    tl_last_error = result.error;
    return -1;
  }

  return 0;
}

__declspec(dllexport) const char *hub_file_get_error() {
  return tl_last_error.c_str();
}

} // extern "C"
