#pragma once
// =============================================================================
// CACHE.H - In-Memory & File Cache with LRU and TTL
// =============================================================================
// Features:
// - LRU eviction when max size reached
// - TTL expiration for entries
// - Thread-safe operations
// - File persistence option
// =============================================================================

#include <chrono>
#include <fstream>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Hub::Cache {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

// =============================================================================
// CACHE ENTRY
// =============================================================================
struct CacheEntry {
  std::string value;
  TimePoint created;
  TimePoint lastAccess;
  int ttlSeconds = 0; // 0 = no expiration

  bool isExpired() const {
    if (ttlSeconds <= 0)
      return false;
    auto now = Clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - created)
               .count() >= ttlSeconds;
  }
};

// =============================================================================
// LRU CACHE
// =============================================================================
class LRUCache {
public:
  explicit LRUCache(size_t maxSize = 1000) : maxSize_(maxSize) {}

  // Set value with optional TTL (seconds, 0 = no expiry)
  void set(const std::string &key, const std::string &value,
           int ttlSeconds = 0) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_.find(key);
    if (it != cache_.end()) {
      // Update existing
      lruList_.erase(it->second.listIt);
      lruList_.push_front(key);
      it->second.entry.value = value;
      it->second.entry.created = Clock::now();
      it->second.entry.lastAccess = Clock::now();
      it->second.entry.ttlSeconds = ttlSeconds;
      it->second.listIt = lruList_.begin();
    } else {
      // Evict if full
      while (cache_.size() >= maxSize_) {
        evictLRU();
      }

      // Insert new
      lruList_.push_front(key);
      CacheItem item;
      item.entry.value = value;
      item.entry.created = Clock::now();
      item.entry.lastAccess = Clock::now();
      item.entry.ttlSeconds = ttlSeconds;
      item.listIt = lruList_.begin();
      cache_[key] = std::move(item);
    }
  }

  // Get value (returns nullopt if not found or expired)
  std::optional<std::string> get(const std::string &key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_.find(key);
    if (it == cache_.end())
      return std::nullopt;

    // Check expiration
    if (it->second.entry.isExpired()) {
      lruList_.erase(it->second.listIt);
      cache_.erase(it);
      return std::nullopt;
    }

    // Move to front (most recently used)
    lruList_.erase(it->second.listIt);
    lruList_.push_front(key);
    it->second.listIt = lruList_.begin();
    it->second.entry.lastAccess = Clock::now();

    return it->second.entry.value;
  }

  // Check if key exists (and not expired)
  bool has(const std::string &key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(key);
    if (it == cache_.end())
      return false;
    return !it->second.entry.isExpired();
  }

  // Remove entry
  bool remove(const std::string &key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(key);
    if (it == cache_.end())
      return false;
    lruList_.erase(it->second.listIt);
    cache_.erase(it);
    return true;
  }

  // Clear all entries
  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    lruList_.clear();
  }

  // Get current size
  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_.size();
  }

  // Remove expired entries
  size_t cleanupExpired() {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t removed = 0;

    for (auto it = cache_.begin(); it != cache_.end();) {
      if (it->second.entry.isExpired()) {
        lruList_.erase(it->second.listIt);
        it = cache_.erase(it);
        removed++;
      } else {
        ++it;
      }
    }

    return removed;
  }

  // Get cache stats
  struct Stats {
    size_t entryCount;
    size_t maxSize;
    size_t hits;
    size_t misses;
  };

  Stats stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {cache_.size(), maxSize_, hits_, misses_};
  }

private:
  void evictLRU() {
    if (lruList_.empty())
      return;
    std::string key = lruList_.back();
    lruList_.pop_back();
    cache_.erase(key);
  }

  struct CacheItem {
    CacheEntry entry;
    std::list<std::string>::iterator listIt;
  };

  mutable std::mutex mutex_;
  std::unordered_map<std::string, CacheItem> cache_;
  std::list<std::string> lruList_;
  size_t maxSize_;
  mutable size_t hits_ = 0;
  mutable size_t misses_ = 0;
};

// =============================================================================
// GLOBAL CACHE INSTANCE
// =============================================================================
inline LRUCache &globalCache() {
  static LRUCache cache(10000); // 10K entries max
  return cache;
}

} // namespace Hub::Cache

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) void hub_cache_set(const char *key, const char *value,
                                         int ttl_seconds);
__declspec(dllexport) const char *hub_cache_get(const char *key);
__declspec(dllexport) int hub_cache_has(const char *key);
__declspec(dllexport) int hub_cache_remove(const char *key);
__declspec(dllexport) void hub_cache_clear();
__declspec(dllexport) int hub_cache_size();
__declspec(dllexport) int hub_cache_cleanup();
}
