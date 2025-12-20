// =============================================================================
// MMAP.CPP - Memory-Mapped File Implementation
// =============================================================================

#include "mmap.h"

extern "C" {

__declspec(dllexport) int hub_mmap_create() {
  return Hub::FileSystem::createMmapFile();
}

__declspec(dllexport) int hub_mmap_open(int handle, const char *filepath,
                                        int mode) {
  if (!filepath)
    return -1;
  auto *mmap = Hub::FileSystem::getMmapFile(handle);
  if (!mmap)
    return -1;

  auto m = Hub::FileSystem::MemoryMappedFile::Mode::READ_ONLY;
  if (mode == 1)
    m = Hub::FileSystem::MemoryMappedFile::Mode::READ_WRITE;
  else if (mode == 2)
    m = Hub::FileSystem::MemoryMappedFile::Mode::COPY_ON_WRITE;

  return mmap->open(filepath, m) ? 0 : -1;
}

__declspec(dllexport) int hub_mmap_create_file(int handle, const char *filepath,
                                               uint64_t size) {
  if (!filepath)
    return -1;
  auto *mmap = Hub::FileSystem::getMmapFile(handle);
  if (!mmap)
    return -1;
  return mmap->create(filepath, static_cast<size_t>(size)) ? 0 : -1;
}

__declspec(dllexport) uint64_t hub_mmap_size(int handle) {
  auto *mmap = Hub::FileSystem::getMmapFile(handle);
  if (!mmap)
    return 0;
  return static_cast<uint64_t>(mmap->size());
}

__declspec(dllexport) int hub_mmap_read(int handle, uint64_t offset,
                                        unsigned char *buffer,
                                        uint64_t length) {
  if (!buffer)
    return -1;
  auto *mmap = Hub::FileSystem::getMmapFile(handle);
  if (!mmap)
    return -1;
  return mmap->read(static_cast<size_t>(offset), buffer,
                    static_cast<size_t>(length))
             ? 0
             : -1;
}

__declspec(dllexport) int hub_mmap_write(int handle, uint64_t offset,
                                         const unsigned char *buffer,
                                         uint64_t length) {
  if (!buffer)
    return -1;
  auto *mmap = Hub::FileSystem::getMmapFile(handle);
  if (!mmap)
    return -1;
  return mmap->write(static_cast<size_t>(offset), buffer,
                     static_cast<size_t>(length))
             ? 0
             : -1;
}

__declspec(dllexport) int hub_mmap_flush(int handle) {
  auto *mmap = Hub::FileSystem::getMmapFile(handle);
  if (!mmap)
    return -1;
  return mmap->flush() ? 0 : -1;
}

__declspec(dllexport) void hub_mmap_close(int handle) {
  auto *mmap = Hub::FileSystem::getMmapFile(handle);
  if (mmap)
    mmap->close();
}

__declspec(dllexport) void hub_mmap_release(int handle) {
  Hub::FileSystem::releaseMmapFile(handle);
}

} // extern "C"
