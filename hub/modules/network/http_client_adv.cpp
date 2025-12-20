// =============================================================================
// HTTP_CLIENT_ADV.CPP - Advanced HTTP Client Implementation
// =============================================================================

#include "http_client_adv.h"
#include <map>
#include <mutex>

static std::mutex g_responseMutex;
static std::map<int, Hub::Network::HttpResponseAdv> g_responses;
static int g_nextHandle = 1;

static int storeResponse(Hub::Network::HttpResponseAdv &&resp) {
  std::lock_guard<std::mutex> lock(g_responseMutex);
  int handle = g_nextHandle++;
  g_responses[handle] = std::move(resp);
  return handle;
}

static Hub::Network::HttpResponseAdv *getResponse(int handle) {
  std::lock_guard<std::mutex> lock(g_responseMutex);
  auto it = g_responses.find(handle);
  return (it != g_responses.end()) ? &it->second : nullptr;
}

extern "C" {

__declspec(dllexport) int hub_http_adv_get(const char *url, int max_retries) {
  if (!url)
    return -1;

  try {
    Hub::Network::RetryPolicy policy;
    policy.maxRetries = max_retries;

    auto response = Hub::Network::HttpClient::get(url, policy);
    return storeResponse(std::move(response));
  } catch (...) {
    Hub::Network::HttpResponseAdv errResp;
    errResp.error = "Exception occurred";
    return storeResponse(std::move(errResp));
  }
}

__declspec(dllexport) int hub_http_adv_post(const char *url, const char *body,
                                            const char *content_type,
                                            int max_retries) {
  if (!url)
    return -1;

  Hub::Network::RetryPolicy policy;
  policy.maxRetries = max_retries;

  std::string ct = content_type ? content_type : "application/json";
  std::string b = body ? body : "";

  auto response = Hub::Network::HttpClient::post(url, b, ct, policy);
  return storeResponse(std::move(response));
}

__declspec(dllexport) int hub_http_adv_status(int handle) {
  auto *resp = getResponse(handle);
  return resp ? resp->statusCode : 0;
}

__declspec(dllexport) const char *hub_http_adv_body(int handle) {
  auto *resp = getResponse(handle);
  return resp ? resp->body.c_str() : "";
}

__declspec(dllexport) int hub_http_adv_attempts(int handle) {
  auto *resp = getResponse(handle);
  return resp ? resp->attempts : 0;
}

__declspec(dllexport) int hub_http_adv_time_ms(int handle) {
  auto *resp = getResponse(handle);
  return resp ? resp->totalTimeMs : 0;
}

__declspec(dllexport) void hub_http_adv_release(int handle) {
  std::lock_guard<std::mutex> lock(g_responseMutex);
  g_responses.erase(handle);
}

__declspec(dllexport) void hub_http_pool_close() {
  Hub::Network::ConnectionPool::instance().closeAll();
}

} // extern "C"
