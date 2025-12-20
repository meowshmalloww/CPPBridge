// =============================================================================
// CACHE.CPP - Cache Implementation
// =============================================================================

#include "cache.h"

static thread_local std::string tl_cache_value;

extern "C" {

__declspec(dllexport) void hub_cache_set(const char *key, const char *value,
                                         int ttl_seconds) {
  if (!key || !value)
    return;
  Hub::Cache::globalCache().set(key, value, ttl_seconds);
}

__declspec(dllexport) const char *hub_cache_get(const char *key) {
  if (!key)
    return "";
  auto val = Hub::Cache::globalCache().get(key);
  if (!val.has_value())
    return "";
  tl_cache_value = *val;
  return tl_cache_value.c_str();
}

__declspec(dllexport) int hub_cache_has(const char *key) {
  if (!key)
    return 0;
  return Hub::Cache::globalCache().has(key) ? 1 : 0;
}

__declspec(dllexport) int hub_cache_remove(const char *key) {
  if (!key)
    return 0;
  return Hub::Cache::globalCache().remove(key) ? 1 : 0;
}

__declspec(dllexport) void hub_cache_clear() {
  Hub::Cache::globalCache().clear();
}

__declspec(dllexport) int hub_cache_size() {
  return static_cast<int>(Hub::Cache::globalCache().size());
}

__declspec(dllexport) int hub_cache_cleanup() {
  return static_cast<int>(Hub::Cache::globalCache().cleanupExpired());
}

} // extern "C"
