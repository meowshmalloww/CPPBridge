#pragma once
// =============================================================================
// KEYVALUE.H - Key-Value Store Module
// =============================================================================
// Fast in-memory key-value store with optional persistence to SQLite.
// =============================================================================

#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace Hub {
namespace Database {

// =============================================================================
// KEY-VALUE STORE
// =============================================================================
class KeyValueStore {
public:
  KeyValueStore() = default;

  // Set a value
  void set(const std::string &key, const std::string &value) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_[key] = value;
  }

  // Get a value
  std::optional<std::string> get(const std::string &key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  // Get with default
  std::string getOrDefault(const std::string &key,
                           const std::string &defaultValue) const {
    auto val = get(key);
    return val.value_or(defaultValue);
  }

  // Check if key exists
  bool exists(const std::string &key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.find(key) != data_.end();
  }

  // Delete a key
  bool remove(const std::string &key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.erase(key) > 0;
  }

  // Get all keys
  std::vector<std::string> keys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    result.reserve(data_.size());
    for (auto it = data_.begin(); it != data_.end(); ++it) {
      result.push_back(it->first);
    }
    return result;
  }

  // Clear all data
  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    data_.clear();
  }

  // Get count
  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
  }

private:
  mutable std::mutex mutex_;
  std::map<std::string, std::string> data_;
};

// Global key-value store instance
inline KeyValueStore globalKVStore;

} // namespace Database
} // namespace Hub

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) void hub_kv_set(const char *key, const char *value);
__declspec(dllexport) const char *hub_kv_get(const char *key);
__declspec(dllexport) int hub_kv_exists(const char *key);
__declspec(dllexport) int hub_kv_remove(const char *key);
__declspec(dllexport) int hub_kv_count();
__declspec(dllexport) void hub_kv_clear();
}
