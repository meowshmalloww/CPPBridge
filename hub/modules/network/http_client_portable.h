#pragma once
// =============================================================================
// HTTP_CLIENT_PORTABLE.H - Cross-Platform HTTP Client
// =============================================================================
// Uses WinInet on Windows, libcurl on Linux/macOS
// =============================================================================

#include <chrono>
#include <cmath>
#include <map>
#include <random>
#include <string>
#include <thread>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define HUB_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#else
#define HUB_PLATFORM_POSIX
// libcurl must be installed: apt install libcurl4-openssl-dev
#include <curl/curl.h>
#endif

namespace Hub::Network {

// =============================================================================
// HTTP RESPONSE
// =============================================================================
struct HttpResponse {
  int statusCode = 0;
  std::string body;
  std::map<std::string, std::string> headers;
  std::string error;
  int attempts = 0;
  int64_t timeMs = 0;
};

// =============================================================================
// RETRY POLICY (Shared)
// =============================================================================
struct HttpRetryPolicy {
  int maxRetries = 3;
  int baseDelayMs = 100;
  int maxDelayMs = 10000;
  double multiplier = 2.0;

  bool shouldRetry(int statusCode) const {
    return statusCode == 408 || statusCode == 429 || statusCode >= 500;
  }

  int calculateDelay(int attempt) const {
    double delay = baseDelayMs * std::pow(multiplier, attempt);
    delay = std::min(delay, static_cast<double>(maxDelayMs));
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.9, 1.1);
    return static_cast<int>(delay * dis(gen));
  }
};

// =============================================================================
// CROSS-PLATFORM HTTP CLIENT
// =============================================================================
class HttpClientPortable {
public:
  static HttpResponse get(const std::string &url,
                          const HttpRetryPolicy &policy = {}) {
    return request("GET", url, "", "", policy);
  }

  static HttpResponse post(const std::string &url, const std::string &body,
                           const std::string &contentType = "application/json",
                           const HttpRetryPolicy &policy = {}) {
    return request("POST", url, body, contentType, policy);
  }

  static HttpResponse request(const std::string &method, const std::string &url,
                              const std::string &body,
                              const std::string &contentType,
                              const HttpRetryPolicy &policy) {
    HttpResponse response;
    auto startTime = std::chrono::steady_clock::now();

    for (int attempt = 0; attempt <= policy.maxRetries; ++attempt) {
      response.attempts = attempt + 1;

      if (attempt > 0) {
        int delay = policy.calculateDelay(attempt - 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
      }

#ifdef HUB_PLATFORM_WINDOWS
      response = executeWindows(method, url, body, contentType);
#else
      response = executeCurl(method, url, body, contentType);
#endif

      if (response.error.empty() && !policy.shouldRetry(response.statusCode)) {
        break;
      }

      if (attempt == policy.maxRetries)
        break;
    }

    auto endTime = std::chrono::steady_clock::now();
    response.timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                          endTime - startTime)
                          .count();

    return response;
  }

private:
#ifdef HUB_PLATFORM_WINDOWS
  static HttpResponse executeWindows(const std::string &method,
                                     const std::string &url,
                                     const std::string &body,
                                     const std::string &contentType) {
    (void)method;
    (void)body;
    (void)contentType; // Used for POST, not GET
    HttpResponse response;

    HINTERNET hInternet = InternetOpenA(
        "CPPBridge/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet) {
      response.error = "InternetOpen failed";
      return response;
    }

    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
    if (url.find("https://") == 0) {
      flags |= INTERNET_FLAG_SECURE;
    }

    HINTERNET hUrl =
        InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, flags, 0);
    if (!hUrl) {
      InternetCloseHandle(hInternet);
      response.error = "InternetOpenUrl failed";
      return response;
    }

    // Get status code
    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    HttpQueryInfoA(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                   &statusCode, &statusSize, nullptr);
    response.statusCode = static_cast<int>(statusCode);

    // Read body
    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) &&
           bytesRead > 0) {
      response.body.append(buffer, bytesRead);
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    return response;
  }
#else
  // libcurl callback
  static size_t writeCallback(void *contents, size_t size, size_t nmemb,
                              void *userp) {
    size_t totalSize = size * nmemb;
    std::string *str = static_cast<std::string *>(userp);
    str->append(static_cast<char *>(contents), totalSize);
    return totalSize;
  }

  static HttpResponse executeCurl(const std::string &method,
                                  const std::string &url,
                                  const std::string &body,
                                  const std::string &contentType) {
    HttpResponse response;

    CURL *curl = curl_easy_init();
    if (!curl) {
      response.error = "curl_easy_init failed";
      return response;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    struct curl_slist *headers = nullptr;

    if (method == "POST") {
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());

      std::string ctHeader = "Content-Type: " + contentType;
      headers = curl_slist_append(headers, ctHeader.c_str());
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      response.error = curl_easy_strerror(res);
    } else {
      long httpCode = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
      response.statusCode = static_cast<int>(httpCode);
    }

    if (headers)
      curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return response;
  }
#endif
};

} // namespace Hub::Network

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_http_portable_get(const char *url, int retries);
__declspec(dllexport) int hub_http_portable_post(const char *url,
                                                 const char *body,
                                                 const char *content_type,
                                                 int retries);
__declspec(dllexport) int hub_http_portable_status(int handle);
__declspec(dllexport) const char *hub_http_portable_body(int handle);
__declspec(dllexport) void hub_http_portable_release(int handle);
}
