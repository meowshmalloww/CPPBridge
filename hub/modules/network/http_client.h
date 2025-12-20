#pragma once
// =============================================================================
// HTTP_CLIENT.H - Enhanced HTTP Client Module
// =============================================================================
// Full HTTP client with GET/POST/PUT/DELETE, headers, and async support.
// Self-contained module with minimal dependencies.
// =============================================================================

#include <iostream>
#include <map>
#include <string>
#include <windows.h>
#include <wininet.h>


#pragma comment(lib, "wininet.lib")

#include "arena.h"

namespace Hub::Network {

// =============================================================================
// SIMPLE ERROR TYPE (Self-contained)
// =============================================================================
struct HttpError {
  int code = 0;
  std::string message;

  bool is_ok() const { return code == 0; }
};

// =============================================================================
// HTTP REQUEST/RESPONSE STRUCTURES
// =============================================================================
struct HttpRequest {
  std::string url;
  std::string method = "GET";
  std::map<std::string, std::string> headers;
  std::string body;
  int timeout_ms = 30000;
  bool follow_redirects = true;
};

struct HttpResponse {
  int status_code = 0;
  std::map<std::string, std::string> headers;
  std::string body;
  HttpError error;

  bool is_ok() const {
    return error.is_ok() && status_code >= 200 && status_code < 300;
  }
};

// Storage arena for async responses
inline Arena<HttpResponse> httpResponseArena;

// =============================================================================
// URL PARSER
// =============================================================================
struct ParsedUrl {
  std::string protocol;
  std::string host;
  int port;
  std::string path;
  bool is_https;

  static ParsedUrl parse(const std::string &url) {
    ParsedUrl result;
    result.port = 80;
    result.is_https = false;
    result.path = "/";

    size_t proto_end = url.find("://");
    size_t host_start = 0;

    if (proto_end != std::string::npos) {
      result.protocol = url.substr(0, proto_end);
      result.is_https = (result.protocol == "https");
      result.port = result.is_https ? 443 : 80;
      host_start = proto_end + 3;
    }

    size_t path_start = url.find('/', host_start);
    size_t port_start = url.find(':', host_start);

    if (port_start != std::string::npos &&
        (path_start == std::string::npos || port_start < path_start)) {
      result.host = url.substr(host_start, port_start - host_start);
      size_t port_end =
          (path_start != std::string::npos) ? path_start : url.length();
      result.port =
          std::stoi(url.substr(port_start + 1, port_end - port_start - 1));
    } else if (path_start != std::string::npos) {
      result.host = url.substr(host_start, path_start - host_start);
    } else {
      result.host = url.substr(host_start);
    }

    if (path_start != std::string::npos) {
      result.path = url.substr(path_start);
    }

    return result;
  }
};

// =============================================================================
// HTTP CLIENT IMPLEMENTATION
// =============================================================================
inline HttpResponse http_request(const HttpRequest &req) {
  std::cout << "[http] Request: " << req.method << " " << req.url << "\n";

  HttpResponse response;
  ParsedUrl parsed = ParsedUrl::parse(req.url);

  // Initialize WinInet
  HINTERNET hInternet = InternetOpenA("UniversalBridge/2.0",
                                      INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
  if (!hInternet) {
    DWORD err = GetLastError();
    response.error.code = static_cast<int>(err);
    response.error.message =
        "InternetOpen failed with error " + std::to_string(err);
    return response;
  }

  // Set timeouts
  DWORD timeout = static_cast<DWORD>(req.timeout_ms);
  InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout,
                     sizeof(timeout));
  InternetSetOptionA(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout,
                     sizeof(timeout));
  InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout,
                     sizeof(timeout));

  // Connect to server
  HINTERNET hConnect = InternetConnectA(
      hInternet, parsed.host.c_str(), static_cast<INTERNET_PORT>(parsed.port),
      NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

  if (!hConnect) {
    DWORD err = GetLastError();
    InternetCloseHandle(hInternet);
    response.error.code = static_cast<int>(err);
    response.error.message = "Connection to " + parsed.host + " failed";
    return response;
  }

  // Open request
  DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
  if (parsed.is_https)
    flags |= INTERNET_FLAG_SECURE;
  if (!req.follow_redirects)
    flags |= INTERNET_FLAG_NO_AUTO_REDIRECT;

  HINTERNET hRequest =
      HttpOpenRequestA(hConnect, req.method.c_str(), parsed.path.c_str(), NULL,
                       NULL, NULL, flags, 0);
  if (!hRequest) {
    DWORD err = GetLastError();
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    response.error.code = static_cast<int>(err);
    response.error.message = "HttpOpenRequest failed";
    return response;
  }

  // Build headers string
  std::string headerStr;
  for (const auto &[key, value] : req.headers) {
    headerStr += key + ": " + value + "\r\n";
  }

  if ((req.method == "POST" || req.method == "PUT") && !req.body.empty()) {
    if (req.headers.find("Content-Type") == req.headers.end()) {
      headerStr += "Content-Type: application/json\r\n";
    }
  }

  // Send request
  BOOL success = HttpSendRequestA(
      hRequest, headerStr.empty() ? NULL : headerStr.c_str(),
      static_cast<DWORD>(headerStr.length()),
      req.body.empty() ? NULL : const_cast<char *>(req.body.c_str()),
      static_cast<DWORD>(req.body.length()));

  if (!success) {
    DWORD err = GetLastError();
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    response.error.code = static_cast<int>(err);
    response.error.message = "HttpSendRequest failed";
    return response;
  }

  // Get status code
  DWORD statusCode = 0;
  DWORD statusCodeSize = sizeof(statusCode);
  HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                 &statusCode, &statusCodeSize, NULL);
  response.status_code = static_cast<int>(statusCode);

  // Read response body
  char buffer[8192];
  DWORD bytesRead;
  while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) &&
         bytesRead > 0) {
    response.body.append(buffer, bytesRead);
  }

  // Cleanup
  InternetCloseHandle(hRequest);
  InternetCloseHandle(hConnect);
  InternetCloseHandle(hInternet);

  std::cout << "[http] Response: " << response.status_code << " ("
            << response.body.size() << " bytes)\n";

  return response;
}

// Convenience functions
inline HttpResponse http_get(const std::string &url) {
  HttpRequest req;
  req.url = url;
  req.method = "GET";
  return http_request(req);
}

inline HttpResponse
http_post(const std::string &url, const std::string &body,
          const std::string &content_type = "application/json") {
  HttpRequest req;
  req.url = url;
  req.method = "POST";
  req.body = body;
  req.headers["Content-Type"] = content_type;
  return http_request(req);
}

} // namespace Hub::Network

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_http_get(const char *url);
__declspec(dllexport) int hub_http_post(const char *url, const char *body,
                                        const char *content_type);
__declspec(dllexport) int hub_http_response_status(int handle);
__declspec(dllexport) int hub_http_response_body_size(int handle);
__declspec(dllexport) const char *hub_http_response_body(int handle);
__declspec(dllexport) void hub_http_response_release(int handle);
}
