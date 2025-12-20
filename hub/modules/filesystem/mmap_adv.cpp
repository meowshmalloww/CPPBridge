// =============================================================================
// MMAP_ADV.CPP - Advanced Memory Mapping Implementation
// =============================================================================

#include "mmap_adv.h"

extern "C" {

__declspec(dllexport) void *hub_chunked_mmap_open(const char *path, int mode,
                                                  uint64_t chunk_size) {
  if (!path)
    return nullptr;

  auto *mmap = new Hub::FileSystem::ChunkedMMap();
  auto m = (mode == 0) ? Hub::FileSystem::ChunkedMMap::Mode::READ_ONLY
                       : Hub::FileSystem::ChunkedMMap::Mode::READ_WRITE;

  size_t cs = (chunk_size > 0)
                  ? static_cast<size_t>(chunk_size)
                  : Hub::FileSystem::ChunkedMMap::DEFAULT_CHUNK_SIZE;

  if (!mmap->open(path, m, cs)) {
    delete mmap;
    return nullptr;
  }

  return mmap;
}

__declspec(dllexport) void *
hub_chunked_mmap_create(const char *path, uint64_t size, uint64_t chunk_size) {
  if (!path || size == 0)
    return nullptr;

  auto *mmap = new Hub::FileSystem::ChunkedMMap();

  size_t cs = (chunk_size > 0)
                  ? static_cast<size_t>(chunk_size)
                  : Hub::FileSystem::ChunkedMMap::DEFAULT_CHUNK_SIZE;

  if (!mmap->create(path, static_cast<size_t>(size), cs)) {
    delete mmap;
    return nullptr;
  }

  return mmap;
}

__declspec(dllexport) void hub_chunked_mmap_close(void *handle) {
  if (!handle)
    return;
  auto *mmap = static_cast<Hub::FileSystem::ChunkedMMap *>(handle);
  delete mmap;
}

__declspec(dllexport) int hub_chunked_mmap_read(void *handle, uint64_t offset,
                                                unsigned char *buffer,
                                                uint64_t length) {
  if (!handle || !buffer)
    return 0;
  auto *mmap = static_cast<Hub::FileSystem::ChunkedMMap *>(handle);
  return mmap->read(static_cast<size_t>(offset), buffer,
                    static_cast<size_t>(length))
             ? 1
             : 0;
}

__declspec(dllexport) int hub_chunked_mmap_write(void *handle, uint64_t offset,
                                                 const unsigned char *buffer,
                                                 uint64_t length) {
  if (!handle || !buffer)
    return 0;
  auto *mmap = static_cast<Hub::FileSystem::ChunkedMMap *>(handle);
  return mmap->write(static_cast<size_t>(offset), buffer,
                     static_cast<size_t>(length))
             ? 1
             : 0;
}

__declspec(dllexport) uint64_t hub_chunked_mmap_size(void *handle) {
  if (!handle)
    return 0;
  auto *mmap = static_cast<Hub::FileSystem::ChunkedMMap *>(handle);
  return static_cast<uint64_t>(mmap->fileSize());
}

__declspec(dllexport) uint64_t hub_chunked_mmap_chunk_count(void *handle) {
  if (!handle)
    return 0;
  auto *mmap = static_cast<Hub::FileSystem::ChunkedMMap *>(handle);
  return static_cast<uint64_t>(mmap->chunkCount());
}

__declspec(dllexport) uint64_t hub_chunked_mmap_recommended_chunk_size() {
  return static_cast<uint64_t>(
      Hub::FileSystem::ChunkedMMap::recommendedChunkSize());
}

} // extern "C"
