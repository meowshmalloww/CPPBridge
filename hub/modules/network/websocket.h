#pragma once
// =============================================================================
// WEBSOCKET.H - WebSocket Client Module
// =============================================================================
// Real-time bidirectional communication using Windows WebSocket API.
// Supports connect, send, receive, and close operations.
// =============================================================================

#include <atomic>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

namespace Hub::Network {

// =============================================================================
// WEBSOCKET MESSAGE TYPES
// =============================================================================
enum class WebSocketMessageType {
  TEXT = 0,
  BINARY = 1,
  CLOSE = 2,
  PING = 3,
  PONG = 4
};

struct WebSocketMessage {
  WebSocketMessageType type = WebSocketMessageType::TEXT;
  std::string data;
  int error_code = 0;
  std::string error_message;

  bool is_ok() const { return error_code == 0; }
};

// =============================================================================
// WEBSOCKET CLIENT
// =============================================================================
class WebSocketClient {
public:
  using MessageCallback = std::function<void(const WebSocketMessage &)>;
  using ConnectCallback =
      std::function<void(bool success, const std::string &error)>;
  using CloseCallback =
      std::function<void(int code, const std::string &reason)>;

  WebSocketClient() = default;
  ~WebSocketClient() { close(); }

  // Connect to WebSocket server
  bool connect(const std::string &url) {
    if (connected_)
      return false;

    std::cout << "[websocket] Connecting to: " << url << "\n";

    // Parse URL
    std::string host, path;
    int port = 443;
    bool secure = true;

    if (!parseUrl(url, host, path, port, secure)) {
      last_error_ = "Invalid URL";
      return false;
    }

    // Initialize WinHTTP
    hSession_ =
        WinHttpOpen(L"UniversalBridge/2.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession_) {
      last_error_ = "WinHttpOpen failed: " + std::to_string(GetLastError());
      return false;
    }

    // Convert host to wide string
    std::wstring wHost(host.begin(), host.end());

    hConnect_ = WinHttpConnect(hSession_, wHost.c_str(),
                               static_cast<INTERNET_PORT>(port), 0);
    if (!hConnect_) {
      last_error_ = "WinHttpConnect failed: " + std::to_string(GetLastError());
      cleanup();
      return false;
    }

    // Convert path to wide string
    std::wstring wPath(path.begin(), path.end());

    DWORD flags = WINHTTP_FLAG_SECURE;
    if (!secure)
      flags = 0;

    hRequest_ = WinHttpOpenRequest(hConnect_, L"GET", wPath.c_str(), NULL,
                                   WINHTTP_NO_REFERER,
                                   WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest_) {
      last_error_ =
          "WinHttpOpenRequest failed: " + std::to_string(GetLastError());
      cleanup();
      return false;
    }

    // Upgrade to WebSocket
    if (!WinHttpSetOption(hRequest_, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL,
                          0)) {
      last_error_ =
          "WebSocket upgrade option failed: " + std::to_string(GetLastError());
      cleanup();
      return false;
    }

    // Send the request
    if (!WinHttpSendRequest(hRequest_, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
      last_error_ =
          "WinHttpSendRequest failed: " + std::to_string(GetLastError());
      cleanup();
      return false;
    }

    // Receive response
    if (!WinHttpReceiveResponse(hRequest_, NULL)) {
      last_error_ =
          "WinHttpReceiveResponse failed: " + std::to_string(GetLastError());
      cleanup();
      return false;
    }

    // Complete WebSocket handshake
    hWebSocket_ = WinHttpWebSocketCompleteUpgrade(hRequest_, 0);
    if (!hWebSocket_) {
      last_error_ =
          "WebSocket upgrade failed: " + std::to_string(GetLastError());
      cleanup();
      return false;
    }

    // Close the request handle (not needed after upgrade)
    WinHttpCloseHandle(hRequest_);
    hRequest_ = NULL;

    connected_ = true;
    shutdown_ = false;

    // Start receive thread
    receiveThread_ = std::thread(&WebSocketClient::receiveLoop, this);

    std::cout << "[websocket] Connected successfully\n";
    return true;
  }

  // Send text message
  bool send(const std::string &message) {
    if (!connected_ || !hWebSocket_)
      return false;

    DWORD err = WinHttpWebSocketSend(
        hWebSocket_, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
        (PVOID)message.data(), static_cast<DWORD>(message.size()));
    if (err != NO_ERROR) {
      last_error_ = "Send failed: " + std::to_string(err);
      return false;
    }

    std::cout << "[websocket] Sent: " << message.substr(0, 50)
              << (message.size() > 50 ? "..." : "") << "\n";
    return true;
  }

  // Send binary message
  bool sendBinary(const std::vector<uint8_t> &data) {
    if (!connected_ || !hWebSocket_)
      return false;

    DWORD err = WinHttpWebSocketSend(
        hWebSocket_, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE,
        (PVOID)data.data(), static_cast<DWORD>(data.size()));
    if (err != NO_ERROR) {
      last_error_ = "Binary send failed: " + std::to_string(err);
      return false;
    }

    return true;
  }

  // Close connection
  void close(int code = 1000, const std::string &reason = "") {
    if (!connected_)
      return;

    shutdown_ = true;

    if (hWebSocket_) {
      WinHttpWebSocketClose(hWebSocket_, static_cast<USHORT>(code),
                            (PVOID)reason.data(),
                            static_cast<DWORD>(reason.size()));
    }

    if (receiveThread_.joinable()) {
      receiveThread_.join();
    }

    cleanup();
    connected_ = false;

    std::cout << "[websocket] Connection closed\n";
  }

  // Set message callback
  void onMessage(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    messageCallback_ = callback;
  }

  // Get next message from queue (polling mode)
  bool pollMessage(WebSocketMessage &msg) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (messageQueue_.empty())
      return false;
    msg = std::move(messageQueue_.front());
    messageQueue_.pop();
    return true;
  }

  bool isConnected() const { return connected_; }
  const std::string &getLastError() const { return last_error_; }

private:
  bool parseUrl(const std::string &url, std::string &host, std::string &path,
                int &port, bool &secure) {
    size_t pos = 0;

    if (url.substr(0, 6) == "wss://") {
      secure = true;
      port = 443;
      pos = 6;
    } else if (url.substr(0, 5) == "ws://") {
      secure = false;
      port = 80;
      pos = 5;
    } else {
      return false;
    }

    size_t pathStart = url.find('/', pos);
    size_t portStart = url.find(':', pos);

    if (portStart != std::string::npos &&
        (pathStart == std::string::npos || portStart < pathStart)) {
      host = url.substr(pos, portStart - pos);
      size_t portEnd =
          (pathStart != std::string::npos) ? pathStart : url.length();
      port = std::stoi(url.substr(portStart + 1, portEnd - portStart - 1));
    } else if (pathStart != std::string::npos) {
      host = url.substr(pos, pathStart - pos);
    } else {
      host = url.substr(pos);
    }

    path = (pathStart != std::string::npos) ? url.substr(pathStart) : "/";
    return true;
  }

  void receiveLoop() {
    std::vector<uint8_t> buffer(65536);

    while (!shutdown_ && connected_) {
      DWORD bytesRead = 0;
      WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;

      DWORD err = WinHttpWebSocketReceive(hWebSocket_, buffer.data(),
                                          static_cast<DWORD>(buffer.size()),
                                          &bytesRead, &bufferType);

      if (err != NO_ERROR) {
        if (!shutdown_) {
          WebSocketMessage msg;
          msg.error_code = static_cast<int>(err);
          msg.error_message = "Receive error: " + std::to_string(err);
          pushMessage(msg);
        }
        break;
      }

      WebSocketMessage msg;

      switch (bufferType) {
      case WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE:
      case WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE:
        msg.type = WebSocketMessageType::TEXT;
        msg.data =
            std::string(reinterpret_cast<char *>(buffer.data()), bytesRead);
        break;

      case WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE:
      case WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE:
        msg.type = WebSocketMessageType::BINARY;
        msg.data =
            std::string(reinterpret_cast<char *>(buffer.data()), bytesRead);
        break;

      case WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE:
        msg.type = WebSocketMessageType::CLOSE;
        connected_ = false;
        break;

      default:
        continue;
      }

      pushMessage(msg);

      if (msg.type == WebSocketMessageType::CLOSE) {
        break;
      }
    }
  }

  void pushMessage(const WebSocketMessage &msg) {
    {
      std::lock_guard<std::mutex> lock(queueMutex_);
      messageQueue_.push(msg);
    }

    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (messageCallback_) {
      messageCallback_(msg);
    }
  }

  void cleanup() {
    if (hWebSocket_) {
      WinHttpCloseHandle(hWebSocket_);
      hWebSocket_ = NULL;
    }
    if (hRequest_) {
      WinHttpCloseHandle(hRequest_);
      hRequest_ = NULL;
    }
    if (hConnect_) {
      WinHttpCloseHandle(hConnect_);
      hConnect_ = NULL;
    }
    if (hSession_) {
      WinHttpCloseHandle(hSession_);
      hSession_ = NULL;
    }
  }

  HINTERNET hSession_ = NULL;
  HINTERNET hConnect_ = NULL;
  HINTERNET hRequest_ = NULL;
  HINTERNET hWebSocket_ = NULL;

  std::atomic<bool> connected_{false};
  std::atomic<bool> shutdown_{false};
  std::string last_error_;

  std::thread receiveThread_;
  std::mutex queueMutex_;
  std::mutex callbackMutex_;
  std::queue<WebSocketMessage> messageQueue_;
  MessageCallback messageCallback_;
};

// Storage for WebSocket clients (using unique_ptr due to non-copyable atomics)
inline std::mutex wsClientMutex;
inline std::map<int, std::unique_ptr<WebSocketClient>> wsClients;
inline std::atomic<int> wsNextHandle{1};

inline int createWebSocketClient() {
  std::lock_guard<std::mutex> lock(wsClientMutex);
  int handle = wsNextHandle++;
  wsClients[handle] = std::make_unique<WebSocketClient>();
  return handle;
}

inline WebSocketClient *getWebSocketClient(int handle) {
  std::lock_guard<std::mutex> lock(wsClientMutex);
  auto it = wsClients.find(handle);
  return (it != wsClients.end()) ? it->second.get() : nullptr;
}

inline void releaseWebSocketClient(int handle) {
  std::lock_guard<std::mutex> lock(wsClientMutex);
  wsClients.erase(handle);
}

} // namespace Hub::Network

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_ws_create();
__declspec(dllexport) int hub_ws_connect(int handle, const char *url);
__declspec(dllexport) int hub_ws_send(int handle, const char *message);
__declspec(dllexport) int hub_ws_poll(int handle, char *buffer,
                                      int buffer_size);
__declspec(dllexport) int hub_ws_is_connected(int handle);
__declspec(dllexport) void hub_ws_close(int handle);
__declspec(dllexport) const char *hub_ws_get_error(int handle);
}
