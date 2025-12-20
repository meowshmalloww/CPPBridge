#pragma once
// =============================================================================
// MMAP.H - Memory-Mapped File I/O
// =============================================================================
// Efficient handling of large files (GBs) without loading into RAM.
// Uses Windows CreateFileMapping / MapViewOfFile APIs.
// =============================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Hub::FileSystem {

// =============================================================================
// MEMORY MAPPED FILE
// =============================================================================
class MemoryMappedFile {
public:
  enum class Mode { READ_ONLY, READ_WRITE, COPY_ON_WRITE };

  MemoryMappedFile() = default;
  ~MemoryMappedFile() { close(); }

  // Non-copyable
  MemoryMappedFile(const MemoryMappedFile &) = delete;
  MemoryMappedFile &operator=(const MemoryMappedFile &) = delete;

  // Movable
  MemoryMappedFile(MemoryMappedFile &&other) noexcept { swap(other); }
  MemoryMappedFile &operator=(MemoryMappedFile &&other) noexcept {
    if (this != &other) {
      close();
      swap(other);
    }
    return *this;
  }

  // Open file for memory mapping
  bool open(const std::string &filepath, Mode mode = Mode::READ_ONLY) {
    close();

    filepath_ = filepath;
    mode_ = mode;

    // File access and share modes
    DWORD desiredAccess = (mode == Mode::READ_ONLY)
                              ? GENERIC_READ
                              : (GENERIC_READ | GENERIC_WRITE);
    DWORD shareMode = FILE_SHARE_READ;
    DWORD createDisposition =
        (mode == Mode::READ_ONLY) ? OPEN_EXISTING : OPEN_ALWAYS;

    // Open file
    hFile_ = CreateFileA(filepath.c_str(), desiredAccess, shareMode, nullptr,
                         createDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile_ == INVALID_HANDLE_VALUE) {
      last_error_ = "Failed to open file: " + std::to_string(GetLastError());
      return false;
    }

    // Get file size
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile_, &fileSize)) {
      last_error_ =
          "Failed to get file size: " + std::to_string(GetLastError());
      close();
      return false;
    }
    file_size_ = static_cast<size_t>(fileSize.QuadPart);

    // If file is empty, we're done (can't map empty file)
    if (file_size_ == 0) {
      return true;
    }

    // Create file mapping
    DWORD protect = PAGE_READONLY;
    if (mode == Mode::READ_WRITE)
      protect = PAGE_READWRITE;
    else if (mode == Mode::COPY_ON_WRITE)
      protect = PAGE_WRITECOPY;

    hMapping_ = CreateFileMappingA(hFile_, nullptr, protect, 0, 0, nullptr);
    if (hMapping_ == nullptr) {
      last_error_ =
          "Failed to create mapping: " + std::to_string(GetLastError());
      close();
      return false;
    }

    // Map view
    DWORD access = FILE_MAP_READ;
    if (mode == Mode::READ_WRITE)
      access = FILE_MAP_ALL_ACCESS;
    else if (mode == Mode::COPY_ON_WRITE)
      access = FILE_MAP_COPY;

    data_ = static_cast<uint8_t *>(MapViewOfFile(hMapping_, access, 0, 0, 0));
    if (data_ == nullptr) {
      last_error_ = "Failed to map view: " + std::to_string(GetLastError());
      close();
      return false;
    }

    mapped_ = true;
    return true;
  }

  // Create new file with specified size
  bool create(const std::string &filepath, size_t size) {
    close();

    filepath_ = filepath;
    mode_ = Mode::READ_WRITE;
    file_size_ = size;

    // Create file
    hFile_ =
        CreateFileA(filepath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile_ == INVALID_HANDLE_VALUE) {
      last_error_ = "Failed to create file: " + std::to_string(GetLastError());
      return false;
    }

    // Set file size
    LARGE_INTEGER li;
    li.QuadPart = static_cast<LONGLONG>(size);
    if (!SetFilePointerEx(hFile_, li, nullptr, FILE_BEGIN) ||
        !SetEndOfFile(hFile_)) {
      last_error_ =
          "Failed to set file size: " + std::to_string(GetLastError());
      close();
      return false;
    }

    // Create mapping
    hMapping_ =
        CreateFileMappingA(hFile_, nullptr, PAGE_READWRITE, 0, 0, nullptr);
    if (hMapping_ == nullptr) {
      last_error_ =
          "Failed to create mapping: " + std::to_string(GetLastError());
      close();
      return false;
    }

    // Map view
    data_ = static_cast<uint8_t *>(
        MapViewOfFile(hMapping_, FILE_MAP_ALL_ACCESS, 0, 0, 0));
    if (data_ == nullptr) {
      last_error_ = "Failed to map view: " + std::to_string(GetLastError());
      close();
      return false;
    }

    mapped_ = true;
    return true;
  }

  void close() {
    if (data_) {
      FlushViewOfFile(data_, 0);
      UnmapViewOfFile(data_);
      data_ = nullptr;
    }
    if (hMapping_) {
      CloseHandle(hMapping_);
      hMapping_ = nullptr;
    }
    if (hFile_ != INVALID_HANDLE_VALUE) {
      CloseHandle(hFile_);
      hFile_ = INVALID_HANDLE_VALUE;
    }
    mapped_ = false;
    file_size_ = 0;
  }

  // Flush changes to disk
  bool flush() {
    if (!mapped_ || !data_)
      return false;
    return FlushViewOfFile(data_, 0) && FlushFileBuffers(hFile_);
  }

  // Access data
  uint8_t *data() { return data_; }
  const uint8_t *data() const { return data_; }
  size_t size() const { return file_size_; }
  bool is_open() const { return mapped_; }
  const std::string &last_error() const { return last_error_; }

  // Read data at offset
  bool read(size_t offset, void *buffer, size_t length) const {
    if (!mapped_ || !data_ || offset + length > file_size_)
      return false;
    std::memcpy(buffer, data_ + offset, length);
    return true;
  }

  // Write data at offset
  bool write(size_t offset, const void *buffer, size_t length) {
    if (!mapped_ || !data_ || mode_ == Mode::READ_ONLY)
      return false;
    if (offset + length > file_size_)
      return false;
    std::memcpy(data_ + offset, buffer, length);
    return true;
  }

  // Get slice of data
  std::vector<uint8_t> slice(size_t offset, size_t length) const {
    if (!mapped_ || !data_ || offset >= file_size_)
      return {};
    length = (std::min)(length, file_size_ - offset);
    return std::vector<uint8_t>(data_ + offset, data_ + offset + length);
  }

private:
  void swap(MemoryMappedFile &other) noexcept {
    std::swap(hFile_, other.hFile_);
    std::swap(hMapping_, other.hMapping_);
    std::swap(data_, other.data_);
    std::swap(file_size_, other.file_size_);
    std::swap(mapped_, other.mapped_);
    std::swap(mode_, other.mode_);
    std::swap(filepath_, other.filepath_);
    std::swap(last_error_, other.last_error_);
  }

  HANDLE hFile_ = INVALID_HANDLE_VALUE;
  HANDLE hMapping_ = nullptr;
  uint8_t *data_ = nullptr;
  size_t file_size_ = 0;
  bool mapped_ = false;
  Mode mode_ = Mode::READ_ONLY;
  std::string filepath_;
  std::string last_error_;
};

// =============================================================================
// MMAP STORAGE
// =============================================================================
inline std::mutex mmapMutex;
inline std::map<int, std::unique_ptr<MemoryMappedFile>> mmapFiles;
inline std::atomic<int> mmapNextHandle{1};

inline int createMmapFile() {
  std::lock_guard<std::mutex> lock(mmapMutex);
  int handle = mmapNextHandle++;
  mmapFiles[handle] = std::make_unique<MemoryMappedFile>();
  return handle;
}

inline MemoryMappedFile *getMmapFile(int handle) {
  std::lock_guard<std::mutex> lock(mmapMutex);
  auto it = mmapFiles.find(handle);
  return (it != mmapFiles.end()) ? it->second.get() : nullptr;
}

inline void releaseMmapFile(int handle) {
  std::lock_guard<std::mutex> lock(mmapMutex);
  mmapFiles.erase(handle);
}

} // namespace Hub::FileSystem

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_mmap_create();
__declspec(dllexport) int hub_mmap_open(int handle, const char *filepath,
                                        int mode);
__declspec(dllexport) int hub_mmap_create_file(int handle, const char *filepath,
                                               uint64_t size);
__declspec(dllexport) uint64_t hub_mmap_size(int handle);
__declspec(dllexport) int hub_mmap_read(int handle, uint64_t offset,
                                        unsigned char *buffer, uint64_t length);
__declspec(dllexport) int hub_mmap_write(int handle, uint64_t offset,
                                         const unsigned char *buffer,
                                         uint64_t length);
__declspec(dllexport) int hub_mmap_flush(int handle);
__declspec(dllexport) void hub_mmap_close(int handle);
__declspec(dllexport) void hub_mmap_release(int handle);
}
