#pragma once
// =============================================================================
// HTTP_CLIENT_ADV.H - Advanced HTTP Client with Connection Pooling
// =============================================================================
// Features:
// - Connection pooling (reuse connections)
// - Exponential backoff retry with jitter
// - Request/response interceptors
// =============================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

#include <chrono>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <thread>

namespace Hub::Network {

// =============================================================================
// RETRY POLICY
// =============================================================================
struct RetryPolicy {
  int maxRetries = 3;
  int baseDelayMs = 100;     // Start with 100ms
  int maxDelayMs = 10000;    // Cap at 10s
  double multiplier = 2.0;   // Double each time
  double jitterFactor = 0.1; // ±10% random jitter

  // Which status codes to retry
  bool shouldRetry(int statusCode) const {
    // Retry server errors and rate limiting
    return statusCode == 408 || // Request Timeout
           statusCode == 429 || // Too Many Requests
           statusCode >= 500;   // Server errors
  }

  // Calculate delay for attempt N (0-indexed)
  int calculateDelay(int attempt) const {
    double delay = baseDelayMs * std::pow(multiplier, attempt);
    delay = std::min(delay, static_cast<double>(maxDelayMs));

    // Add jitter
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(1.0 - jitterFactor,
                                         1.0 + jitterFactor);

    return static_cast<int>(delay * dis(gen));
  }
};

// =============================================================================
// CONNECTION POOL
// =============================================================================
class ConnectionPool {
public:
  static ConnectionPool &instance() {
    static ConnectionPool pool;
    return pool;
  }

  // Get or create session for host
  HINTERNET getSession() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!hInternet_) {
      hInternet_ =
          InternetOpenA("UniversalBridge/2.0", INTERNET_OPEN_TYPE_DIRECT,
                        nullptr, nullptr, 0);

      if (hInternet_) {
        // Set connection pool size
        DWORD poolSize = 10;
        InternetSetOptionA(hInternet_, INTERNET_OPTION_MAX_CONNS_PER_SERVER,
                           &poolSize, sizeof(poolSize));
        InternetSetOptionA(hInternet_, INTERNET_OPTION_MAX_CONNS_PER_1_0_SERVER,
                           &poolSize, sizeof(poolSize));
      }
    }

    return hInternet_;
  }

  // Get connection to specific host (pooled)
  HINTERNET getConnection(const std::string &host, int port, bool https) {
    (void)https; // Protocol handled by flags, not port
    std::lock_guard<std::mutex> lock(mutex_);

    std::string key = host + ":" + std::to_string(port);

    // Check for existing connection
    auto it = connections_.find(key);
    if (it != connections_.end() && it->second != nullptr) {
      return it->second;
    }

    // Create new connection
    HINTERNET hSession = getSession();
    if (!hSession)
      return nullptr;

    HINTERNET hConnect = InternetConnectA(
        hSession, host.c_str(), static_cast<INTERNET_PORT>(port), nullptr,
        nullptr, INTERNET_SERVICE_HTTP, 0, 0);

    if (hConnect) {
      connections_[key] = hConnect;
    }

    return hConnect;
  }

  void closeAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto &[key, handle] : connections_) {
      if (handle)
        InternetCloseHandle(handle);
    }
    connections_.clear();

    if (hInternet_) {
      InternetCloseHandle(hInternet_);
      hInternet_ = nullptr;
    }
  }

  ~ConnectionPool() { closeAll(); }

private:
  ConnectionPool() = default;

  HINTERNET hInternet_ = nullptr;
  std::map<std::string, HINTERNET> connections_;
  std::mutex mutex_;
};

// =============================================================================
// ADVANCED HTTP REQUEST/RESPONSE
// =============================================================================
struct HttpRequestAdv {
  std::string url;
  std::string method = "GET";
  std::map<std::string, std::string> headers;
  std::string body;
  int timeoutMs = 30000;
  bool followRedirects = true;
  RetryPolicy retryPolicy;
  bool usePool = true;
};

struct HttpResponseAdv {
  int statusCode = 0;
  std::map<std::string, std::string> headers;
  std::string body;
  int attempts = 0;
  int totalTimeMs = 0;
  std::string error;

  bool isOk() const {
    return error.empty() && statusCode >= 200 && statusCode < 300;
  }
};

// =============================================================================
// URL PARSER
// =============================================================================
struct UrlParts {
  std::string host;
  int port = 80;
  std::string path = "/";
  bool https = false;

  static UrlParts parse(const std::string &url) {
    UrlParts result;

    size_t protoEnd = url.find("://");
    size_t hostStart = 0;

    if (protoEnd != std::string::npos) {
      std::string proto = url.substr(0, protoEnd);
      result.https = (proto == "https");
      result.port = result.https ? 443 : 80;
      hostStart = protoEnd + 3;
    }

    size_t pathStart = url.find('/', hostStart);
    size_t portStart = url.find(':', hostStart);

    if (portStart != std::string::npos &&
        (pathStart == std::string::npos || portStart < pathStart)) {
      result.host = url.substr(hostStart, portStart - hostStart);
      size_t portEnd =
          (pathStart != std::string::npos) ? pathStart : url.length();
      result.port =
          std::stoi(url.substr(portStart + 1, portEnd - portStart - 1));
    } else if (pathStart != std::string::npos) {
      result.host = url.substr(hostStart, pathStart - hostStart);
    } else {
      result.host = url.substr(hostStart);
    }

    if (pathStart != std::string::npos) {
      result.path = url.substr(pathStart);
    }

    return result;
  }
};

// =============================================================================
// ADVANCED HTTP CLIENT
// =============================================================================
class HttpClient {
public:
  static HttpResponseAdv request(const HttpRequestAdv &req) {
    auto startTime = std::chrono::steady_clock::now();
    HttpResponseAdv response;

    UrlParts url = UrlParts::parse(req.url);

    for (int attempt = 0; attempt <= req.retryPolicy.maxRetries; ++attempt) {
      response.attempts = attempt + 1;

      if (attempt > 0) {
        int delay = req.retryPolicy.calculateDelay(attempt - 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
      }

      HttpResponseAdv attemptResponse = executeRequest(req, url);
      response = attemptResponse;
      response.attempts = attempt + 1; // Preserve attempt count

      // Success - no error and not a retryable status
      if (response.error.empty() &&
          !req.retryPolicy.shouldRetry(response.statusCode)) {
        break;
      }

      // Max retries reached
      if (attempt == req.retryPolicy.maxRetries) {
        break;
      }

      // Otherwise continue retrying (connection error or retryable status)
    }

    auto endTime = std::chrono::steady_clock::now();
    response.totalTimeMs =
        static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
                             endTime - startTime)
                             .count());

    return response;
  }

  // Convenience methods
  static HttpResponseAdv get(const std::string &url,
                             const RetryPolicy &retry = {}) {
    HttpRequestAdv req;
    req.url = url;
    req.method = "GET";
    req.retryPolicy = retry;
    return request(req);
  }

  static HttpResponseAdv
  post(const std::string &url, const std::string &body,
       const std::string &contentType = "application/json",
       const RetryPolicy &retry = {}) {
    HttpRequestAdv req;
    req.url = url;
    req.method = "POST";
    req.body = body;
    req.headers["Content-Type"] = contentType;
    req.retryPolicy = retry;
    return request(req);
  }

private:
  static HttpResponseAdv executeRequest(const HttpRequestAdv &req,
                                        const UrlParts &url) {
    HttpResponseAdv response;

    // Get connection from pool
    HINTERNET hConnect = req.usePool ? ConnectionPool::instance().getConnection(
                                           url.host, url.port, url.https)
                                     : nullptr;

    HINTERNET hSession = nullptr;
    bool ownConnection = false;

    if (!hConnect) {
      // Fallback to non-pooled connection
      hSession = InternetOpenA("UniversalBridge/2.0", INTERNET_OPEN_TYPE_DIRECT,
                               nullptr, nullptr, 0);
      if (!hSession) {
        response.error = "Failed to create session";
        return response;
      }

      hConnect = InternetConnectA(hSession, url.host.c_str(),
                                  static_cast<INTERNET_PORT>(url.port), nullptr,
                                  nullptr, INTERNET_SERVICE_HTTP, 0, 0);
      if (!hConnect) {
        InternetCloseHandle(hSession);
        response.error = "Failed to connect to " + url.host;
        return response;
      }
      ownConnection = true;
    }

    // Set timeout
    DWORD timeout = static_cast<DWORD>(req.timeoutMs);
    InternetSetOptionA(hConnect, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout,
                       sizeof(timeout));
    InternetSetOptionA(hConnect, INTERNET_OPTION_SEND_TIMEOUT, &timeout,
                       sizeof(timeout));
    InternetSetOptionA(hConnect, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout,
                       sizeof(timeout));

    // Open request
    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
    if (url.https)
      flags |= INTERNET_FLAG_SECURE;
    if (!req.followRedirects)
      flags |= INTERNET_FLAG_NO_AUTO_REDIRECT;

    HINTERNET hRequest =
        HttpOpenRequestA(hConnect, req.method.c_str(), url.path.c_str(),
                         nullptr, nullptr, nullptr, flags, 0);
    if (!hRequest) {
      if (ownConnection) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
      }
      response.error = "Failed to open request";
      return response;
    }

    // Build headers
    std::string headerStr;
    for (const auto &[key, value] : req.headers) {
      headerStr += key + ": " + value + "\r\n";
    }

    // Send request
    BOOL success = HttpSendRequestA(
        hRequest, headerStr.empty() ? nullptr : headerStr.c_str(),
        static_cast<DWORD>(headerStr.length()),
        req.body.empty() ? nullptr : const_cast<char *>(req.body.c_str()),
        static_cast<DWORD>(req.body.length()));

    if (!success) {
      InternetCloseHandle(hRequest);
      if (ownConnection) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
      }
      response.error = "Request failed: " + std::to_string(GetLastError());
      return response;
    }

    // Get status code
    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                   &statusCode, &statusSize, nullptr);
    response.statusCode = static_cast<int>(statusCode);

    // Read body
    char buffer[8192];
    DWORD bytesRead;
    while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) &&
           bytesRead > 0) {
      response.body.append(buffer, bytesRead);
    }

    // Cleanup
    InternetCloseHandle(hRequest);
    if (ownConnection) {
      InternetCloseHandle(hConnect);
      InternetCloseHandle(hSession);
    }

    return response;
  }
};

} // namespace Hub::Network

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_http_adv_get(const char *url, int max_retries);
__declspec(dllexport) int hub_http_adv_post(const char *url, const char *body,
                                            const char *content_type,
                                            int max_retries);
__declspec(dllexport) int hub_http_adv_status(int handle);
__declspec(dllexport) const char *hub_http_adv_body(int handle);
__declspec(dllexport) int hub_http_adv_attempts(int handle);
__declspec(dllexport) int hub_http_adv_time_ms(int handle);
__declspec(dllexport) void hub_http_adv_release(int handle);
__declspec(dllexport) void hub_http_pool_close();
}
