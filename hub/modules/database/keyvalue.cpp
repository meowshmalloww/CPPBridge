// =============================================================================
// KEYVALUE.CPP - Key-Value Store Implementation
// =============================================================================

#include "keyvalue.h"

// Thread-local storage for returning values
static thread_local std::string tl_value;

extern "C" {

__declspec(dllexport) void hub_kv_set(const char *key, const char *value) {
  if (!key || !value)
    return;
  Hub::Database::globalKVStore.set(key, value);
}

__declspec(dllexport) const char *hub_kv_get(const char *key) {
  if (!key)
    return "";
  auto val = Hub::Database::globalKVStore.get(key);
  if (val) {
    tl_value = *val;
    return tl_value.c_str();
  }
  return "";
}

__declspec(dllexport) int hub_kv_exists(const char *key) {
  if (!key)
    return 0;
  return Hub::Database::globalKVStore.exists(key) ? 1 : 0;
}

__declspec(dllexport) int hub_kv_remove(const char *key) {
  if (!key)
    return 0;
  return Hub::Database::globalKVStore.remove(key) ? 1 : 0;
}

__declspec(dllexport) int hub_kv_count() {
  return static_cast<int>(Hub::Database::globalKVStore.size());
}

__declspec(dllexport) void hub_kv_clear() {
  Hub::Database::globalKVStore.clear();
}

} // extern "C"
