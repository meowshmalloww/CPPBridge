// =============================================================================
// HTTP_CLIENT.CPP - HTTP Client Implementation
// =============================================================================

#include "http_client.h"

// Thread-local storage for returning body strings safely
static thread_local std::string tl_response_body;

extern "C" {

__declspec(dllexport) int hub_http_get(const char *url) {
  if (!url)
    return -1;

  auto response = Hub::Network::http_get(url);
  return static_cast<int>(
      Hub::Network::httpResponseArena.insert(response).index);
}

__declspec(dllexport) int hub_http_post(const char *url, const char *body,
                                        const char *content_type) {
  if (!url)
    return -1;

  std::string bodyStr = body ? body : "";
  std::string ctStr = content_type ? content_type : "application/json";

  auto response = Hub::Network::http_post(url, bodyStr, ctStr);
  return static_cast<int>(
      Hub::Network::httpResponseArena.insert(response).index);
}

__declspec(dllexport) int hub_http_response_status(int handle) {
  auto *resp =
      Hub::Network::httpResponseArena.access({static_cast<uint32_t>(handle)});
  if (!resp)
    return -1;
  return resp->status_code;
}

__declspec(dllexport) int hub_http_response_body_size(int handle) {
  auto *resp =
      Hub::Network::httpResponseArena.access({static_cast<uint32_t>(handle)});
  if (!resp)
    return 0;
  return static_cast<int>(resp->body.size());
}

__declspec(dllexport) const char *hub_http_response_body(int handle) {
  auto *resp =
      Hub::Network::httpResponseArena.access({static_cast<uint32_t>(handle)});
  if (!resp)
    return "";
  tl_response_body = resp->body;
  return tl_response_body.c_str();
}

__declspec(dllexport) void hub_http_response_release(int handle) {
  Hub::Network::httpResponseArena.release({static_cast<uint32_t>(handle)});
}

} // extern "C"
