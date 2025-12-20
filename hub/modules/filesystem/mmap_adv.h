#pragma once
// =============================================================================
// MMAP_ADV.H - Advanced Memory Mapping with Auto-Chunking
// =============================================================================
// Features:
// - Auto-chunking for files larger than RAM
// - Sliding window access for huge files
// - Cross-platform (Windows + Linux/macOS)
// =============================================================================

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

// Platform detection
#ifdef _WIN32
#define HUB_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#define HUB_PLATFORM_POSIX
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace Hub::FileSystem {

// =============================================================================
// CHUNKED MEMORY-MAPPED FILE
// =============================================================================
// For files larger than available RAM, this class uses a sliding window
// approach, mapping only a portion of the file at a time.
// =============================================================================
class ChunkedMMap {
public:
  enum class Mode { READ_ONLY, READ_WRITE };

  // Default chunk size: 64MB (adjustable)
  static constexpr size_t DEFAULT_CHUNK_SIZE = 64 * 1024 * 1024;

  ChunkedMMap() = default;
  ~ChunkedMMap() { close(); }

  // Non-copyable
  ChunkedMMap(const ChunkedMMap &) = delete;
  ChunkedMMap &operator=(const ChunkedMMap &) = delete;

  // Open file with chunked mapping
  bool open(const std::string &filepath, Mode mode = Mode::READ_ONLY,
            size_t chunkSize = DEFAULT_CHUNK_SIZE) {
    close();

    filepath_ = filepath;
    mode_ = mode;
    chunkSize_ = chunkSize;

#ifdef HUB_PLATFORM_WINDOWS
    // Windows implementation
    DWORD access = (mode == Mode::READ_ONLY) ? GENERIC_READ
                                             : (GENERIC_READ | GENERIC_WRITE);
    DWORD share = FILE_SHARE_READ;
    DWORD create = OPEN_EXISTING;

    hFile_ = CreateFileA(filepath.c_str(), access, share, nullptr, create,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile_ == INVALID_HANDLE_VALUE) {
      lastError_ = "Failed to open file";
      return false;
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(hFile_, &size)) {
      lastError_ = "Failed to get file size";
      close();
      return false;
    }
    fileSize_ = static_cast<size_t>(size.QuadPart);

    DWORD protect = (mode == Mode::READ_ONLY) ? PAGE_READONLY : PAGE_READWRITE;
    hMapping_ = CreateFileMappingA(hFile_, nullptr, protect, 0, 0, nullptr);
    if (!hMapping_) {
      lastError_ = "Failed to create mapping";
      close();
      return false;
    }
#else
    // POSIX implementation (Linux/macOS)
    int flags = (mode == Mode::READ_ONLY) ? O_RDONLY : O_RDWR;
    fd_ = ::open(filepath.c_str(), flags);
    if (fd_ < 0) {
      lastError_ = "Failed to open file";
      return false;
    }

    struct stat st;
    if (fstat(fd_, &st) != 0) {
      lastError_ = "Failed to get file size";
      close();
      return false;
    }
    fileSize_ = static_cast<size_t>(st.st_size);
#endif

    isOpen_ = true;
    return true;
  }

  // Create new file with specified size
  bool create(const std::string &filepath, size_t size,
              size_t chunkSize = DEFAULT_CHUNK_SIZE) {
    close();

    filepath_ = filepath;
    mode_ = Mode::READ_WRITE;
    chunkSize_ = chunkSize;
    fileSize_ = size;

#ifdef HUB_PLATFORM_WINDOWS
    hFile_ =
        CreateFileA(filepath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile_ == INVALID_HANDLE_VALUE) {
      lastError_ = "Failed to create file";
      return false;
    }

    LARGE_INTEGER li;
    li.QuadPart = static_cast<LONGLONG>(size);
    if (!SetFilePointerEx(hFile_, li, nullptr, FILE_BEGIN) ||
        !SetEndOfFile(hFile_)) {
      lastError_ = "Failed to set file size";
      close();
      return false;
    }

    hMapping_ =
        CreateFileMappingA(hFile_, nullptr, PAGE_READWRITE, 0, 0, nullptr);
    if (!hMapping_) {
      lastError_ = "Failed to create mapping";
      close();
      return false;
    }
#else
    fd_ = ::open(filepath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd_ < 0) {
      lastError_ = "Failed to create file";
      return false;
    }

    if (ftruncate(fd_, static_cast<off_t>(size)) != 0) {
      lastError_ = "Failed to set file size";
      close();
      return false;
    }
#endif

    isOpen_ = true;
    return true;
  }

  void close() {
    unmapCurrentChunk();

#ifdef HUB_PLATFORM_WINDOWS
    if (hMapping_) {
      CloseHandle(hMapping_);
      hMapping_ = nullptr;
    }
    if (hFile_ != INVALID_HANDLE_VALUE) {
      CloseHandle(hFile_);
      hFile_ = INVALID_HANDLE_VALUE;
    }
#else
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
#endif

    isOpen_ = false;
    fileSize_ = 0;
  }

  // Map a specific chunk (auto-called by read/write)
  bool mapChunk(size_t chunkIndex) {
    if (chunkIndex == currentChunkIndex_ && currentChunkData_) {
      return true; // Already mapped
    }

    unmapCurrentChunk();

    size_t offset = chunkIndex * chunkSize_;
    if (offset >= fileSize_) {
      return false;
    }

    size_t actualSize = std::min(chunkSize_, fileSize_ - offset);

#ifdef HUB_PLATFORM_WINDOWS
    DWORD access =
        (mode_ == Mode::READ_ONLY) ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;
    DWORD offsetHigh = static_cast<DWORD>(offset >> 32);
    DWORD offsetLow = static_cast<DWORD>(offset & 0xFFFFFFFF);

    currentChunkData_ = static_cast<uint8_t *>(
        MapViewOfFile(hMapping_, access, offsetHigh, offsetLow, actualSize));
#else
    int prot =
        (mode_ == Mode::READ_ONLY) ? PROT_READ : (PROT_READ | PROT_WRITE);
    currentChunkData_ =
        static_cast<uint8_t *>(mmap(nullptr, actualSize, prot, MAP_SHARED, fd_,
                                    static_cast<off_t>(offset)));
    if (currentChunkData_ == MAP_FAILED) {
      currentChunkData_ = nullptr;
    }
#endif

    if (!currentChunkData_) {
      lastError_ = "Failed to map chunk";
      return false;
    }

    currentChunkIndex_ = chunkIndex;
    currentChunkSize_ = actualSize;
    currentChunkOffset_ = offset;
    return true;
  }

  void unmapCurrentChunk() {
    if (currentChunkData_) {
#ifdef HUB_PLATFORM_WINDOWS
      FlushViewOfFile(currentChunkData_, 0);
      UnmapViewOfFile(currentChunkData_);
#else
      msync(currentChunkData_, currentChunkSize_, MS_SYNC);
      munmap(currentChunkData_, currentChunkSize_);
#endif
      currentChunkData_ = nullptr;
      currentChunkIndex_ = SIZE_MAX;
      currentChunkSize_ = 0;
      currentChunkOffset_ = 0;
    }
  }

  // Read data (auto-chunks)
  bool read(size_t offset, void *buffer, size_t length) {
    if (!isOpen_ || offset + length > fileSize_)
      return false;

    uint8_t *dest = static_cast<uint8_t *>(buffer);
    size_t remaining = length;
    size_t currentOffset = offset;

    while (remaining > 0) {
      size_t chunkIndex = currentOffset / chunkSize_;
      if (!mapChunk(chunkIndex))
        return false;

      size_t offsetInChunk = currentOffset - currentChunkOffset_;
      size_t available = currentChunkSize_ - offsetInChunk;
      size_t toCopy = std::min(remaining, available);

      std::memcpy(dest, currentChunkData_ + offsetInChunk, toCopy);

      dest += toCopy;
      currentOffset += toCopy;
      remaining -= toCopy;
    }

    return true;
  }

  // Write data (auto-chunks)
  bool write(size_t offset, const void *buffer, size_t length) {
    if (!isOpen_ || mode_ == Mode::READ_ONLY || offset + length > fileSize_) {
      return false;
    }

    const uint8_t *src = static_cast<const uint8_t *>(buffer);
    size_t remaining = length;
    size_t currentOffset = offset;

    while (remaining > 0) {
      size_t chunkIndex = currentOffset / chunkSize_;
      if (!mapChunk(chunkIndex))
        return false;

      size_t offsetInChunk = currentOffset - currentChunkOffset_;
      size_t available = currentChunkSize_ - offsetInChunk;
      size_t toCopy = std::min(remaining, available);

      std::memcpy(currentChunkData_ + offsetInChunk, src, toCopy);

      src += toCopy;
      currentOffset += toCopy;
      remaining -= toCopy;
    }

    return true;
  }

  // Stream processing for huge files
  using ChunkProcessor =
      std::function<bool(const uint8_t *data, size_t size, size_t offset)>;

  bool processChunks(ChunkProcessor processor) {
    if (!isOpen_)
      return false;

    size_t numChunks = (fileSize_ + chunkSize_ - 1) / chunkSize_;

    for (size_t i = 0; i < numChunks; ++i) {
      if (!mapChunk(i))
        return false;

      if (!processor(currentChunkData_, currentChunkSize_,
                     currentChunkOffset_)) {
        return false; // Processor requested stop
      }
    }

    return true;
  }

  // Getters
  size_t fileSize() const { return fileSize_; }
  size_t chunkSize() const { return chunkSize_; }
  size_t chunkCount() const {
    return (fileSize_ + chunkSize_ - 1) / chunkSize_;
  }
  bool isOpen() const { return isOpen_; }
  const std::string &lastError() const { return lastError_; }

  // Memory stats
  static size_t recommendedChunkSize() {
#ifdef HUB_PLATFORM_WINDOWS
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
      // Use ~10% of available physical memory, capped at 256MB
      size_t recommended = static_cast<size_t>(status.ullAvailPhys / 10);
      return std::min(recommended, size_t(256) * 1024 * 1024);
    }
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long pageSize = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && pageSize > 0) {
      size_t totalMem =
          static_cast<size_t>(pages) * static_cast<size_t>(pageSize);
      size_t recommended = totalMem / 10;
      return std::min(recommended, size_t(256) * 1024 * 1024);
    }
#endif
    return DEFAULT_CHUNK_SIZE;
  }

private:
#ifdef HUB_PLATFORM_WINDOWS
  HANDLE hFile_ = INVALID_HANDLE_VALUE;
  HANDLE hMapping_ = nullptr;
#else
  int fd_ = -1;
#endif

  std::string filepath_;
  Mode mode_ = Mode::READ_ONLY;
  size_t fileSize_ = 0;
  size_t chunkSize_ = DEFAULT_CHUNK_SIZE;
  bool isOpen_ = false;
  std::string lastError_;

  // Current chunk state
  uint8_t *currentChunkData_ = nullptr;
  size_t currentChunkIndex_ = SIZE_MAX;
  size_t currentChunkSize_ = 0;
  size_t currentChunkOffset_ = 0;
};

} // namespace Hub::FileSystem

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) void *hub_chunked_mmap_open(const char *path, int mode,
                                                  uint64_t chunk_size);
__declspec(dllexport) void *
hub_chunked_mmap_create(const char *path, uint64_t size, uint64_t chunk_size);
__declspec(dllexport) void hub_chunked_mmap_close(void *handle);
__declspec(dllexport) int hub_chunked_mmap_read(void *handle, uint64_t offset,
                                                unsigned char *buffer,
                                                uint64_t length);
__declspec(dllexport) int hub_chunked_mmap_write(void *handle, uint64_t offset,
                                                 const unsigned char *buffer,
                                                 uint64_t length);
__declspec(dllexport) uint64_t hub_chunked_mmap_size(void *handle);
__declspec(dllexport) uint64_t hub_chunked_mmap_chunk_count(void *handle);
__declspec(dllexport) uint64_t hub_chunked_mmap_recommended_chunk_size();
}
