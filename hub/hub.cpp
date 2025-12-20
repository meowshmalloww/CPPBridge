// =============================================================================
// HUB.CPP - Universal Bridge Main Switchboard
// =============================================================================
// Central coordinator that initializes subsystems and provides legacy API.
// All new features should be implemented in modules/ directory.
// =============================================================================

#include "arena.h"
#include "core/async.h"
#include "core/logger.h"
#include "core/types.h"
#include "modules/filesystem/file_manager.h"
#include "modules/network/http_client.h"

#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

namespace Hub {

// =============================================================================
// GLOBAL ARENAS (Legacy - maintained for backward compatibility)
// =============================================================================
Arena<SQLConfig> sqlArena;
Arena<BigData> bigDataArena;
Arena<ServerRequest> requestArena;
Arena<ServerResponse> responseArena;
Arena<CameraFrame> frameArena;

// =============================================================================
// INITIALIZATION
// =============================================================================
static bool g_initialized = false;

void initialize() {
  if (g_initialized)
    return;

  // Initialize logger
  Logger::instance().set_console_output(true);
  Logger::instance().set_min_level(LogLevel::LEVEL_DEBUG);
  LOG_INFO("hub", "UniversalBridge v2.0.0 initializing...");

  // Initialize async runner with default thread count
  AsyncRunner::instance().init();

  g_initialized = true;
  LOG_INFO("hub", "Initialization complete");
}

void shutdown() {
  if (!g_initialized)
    return;

  LOG_INFO("hub", "Shutting down...");
  AsyncRunner::instance().shutdown();
  g_initialized = false;
}

// =============================================================================
// LEGACY HTTP IMPLEMENTATION (for backward compatibility)
// Uses WinInet directly - new code should use Network::http_request()
// =============================================================================
std::string real_http_get(const std::string &server, const std::string &path) {
  HINTERNET hInt = InternetOpenA("UniversalBridge/1.0",
                                 INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
  if (!hInt)
    return "";

  HINTERNET hConn =
      InternetConnectA(hInt, server.c_str(), INTERNET_DEFAULT_HTTPS_PORT, NULL,
                       NULL, INTERNET_SERVICE_HTTP, 0, 0);
  if (!hConn) {
    InternetCloseHandle(hInt);
    return "";
  }

  HINTERNET hReq = HttpOpenRequestA(hConn, "GET", path.c_str(), NULL, NULL,
                                    NULL, INTERNET_FLAG_SECURE, 0);
  if (!hReq) {
    InternetCloseHandle(hConn);
    InternetCloseHandle(hInt);
    return "";
  }

  if (HttpSendRequestA(hReq, NULL, 0, NULL, 0)) {
    std::string content;
    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hReq, buffer, sizeof(buffer), &bytesRead) &&
           bytesRead > 0) {
      content.append(buffer, bytesRead);
    }
    InternetCloseHandle(hReq);
    InternetCloseHandle(hConn);
    InternetCloseHandle(hInt);
    return content;
  }

  InternetCloseHandle(hReq);
  InternetCloseHandle(hConn);
  InternetCloseHandle(hInt);
  return "";
}

extern "C" {
// =============================================================================
// SYSTEM LIFECYCLE
// =============================================================================
__declspec(dllexport) void hub_init() { initialize(); }

__declspec(dllexport) void hub_shutdown() { shutdown(); }

__declspec(dllexport) const char *hub_version() { return "2.0.0"; }

// =============================================================================
// LEGACY API (maintained for backward compatibility with test_connection.py)
// =============================================================================
__declspec(dllexport) void release_handle(int index) {
  responseArena.release({(uint32_t)index});
}

__declspec(dllexport) int save_file_to_disk(const char *filename,
                                            const char *content) {
  if (!filename || !content)
    return 0;
  auto result = FileSystem::write_file(filename, content);
  return result.is_ok() ? 1 : 0;
}

__declspec(dllexport) int send_server_request(int requestHandle) {
  ServerRequest *req = requestArena.access({(uint32_t)requestHandle});
  if (!req)
    return -1;

  LOG_INFO("hub", "Legacy request to: " + req->url);

  // Parse URL
  std::string url = req->url;
  std::string domain = "www.google.com";
  std::string path = "/";

  size_t protocol = url.find("://");
  if (protocol != std::string::npos) {
    size_t pathStart = url.find("/", protocol + 3);
    if (pathStart != std::string::npos) {
      domain = url.substr(protocol + 3, pathStart - (protocol + 3));
      path = url.substr(pathStart);
    } else {
      domain = url.substr(protocol + 3);
    }
  }

  std::string result = real_http_get(domain, path);

  ServerResponse resp;
  if (!result.empty()) {
    resp.statusCode = 200;
    resp.body = result;
    LOG_INFO("hub", "Downloaded " + std::to_string(result.size()) + " bytes");
  } else {
    resp.statusCode = 500;
    resp.body = "Connection Failed";
    LOG_ERROR("hub", "Failed to connect to " + domain);
  }

  return (int)responseArena.insert(resp).index;
}

__declspec(dllexport) int create_server_request(const char *url,
                                                const char *method) {
  if (!g_initialized)
    initialize();

  ServerRequest req;
  req.url = std::string(url ? url : "");
  req.method = std::string(method ? method : "GET");
  return (int)requestArena.insert(req).index;
}
}

} // namespace Hub