#pragma once
// =============================================================================
// WEBSOCKET_ADV.H - Advanced WebSocket Client with Auto-Reconnect
// =============================================================================
// Features:
// - Auto-reconnect with exponential backoff
// - Binary message support with C API
// - Connection state callbacks
// =============================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace Hub::Network {

// =============================================================================
// RECONNECT POLICY
// =============================================================================
struct ReconnectPolicy {
  bool enabled = true;
  int maxAttempts = 10;
  int baseDelayMs = 1000;
  int maxDelayMs = 30000;
  double multiplier = 1.5;

  int calculateDelay(int attempt) const {
    double delay = baseDelayMs * std::pow(multiplier, attempt);
    return static_cast<int>(std::min(delay, static_cast<double>(maxDelayMs)));
  }
};

// =============================================================================
// MESSAGE TYPE
// =============================================================================
enum class WSMsgType { TEXT, BINARY, CLOSE, ERR };

struct WSMsg {
  WSMsgType type = WSMsgType::TEXT;
  std::vector<uint8_t> data;
  int errCode = 0;
  std::string errMessage;

  std::string asText() const { return std::string(data.begin(), data.end()); }
};

// =============================================================================
// ADVANCED WEBSOCKET CLIENT
// =============================================================================
class WebSocketAdv {
public:
  using MessageHandler = std::function<void(const WSMsg &)>;

  WebSocketAdv() = default;
  ~WebSocketAdv() { disconnect(); }

  bool connect(const std::string &url, const ReconnectPolicy &policy = {}) {
    std::lock_guard<std::mutex> lock(mutex_);

    url_ = url;
    policy_ = policy;
    shouldReconnect_ = policy.enabled;
    attempts_ = 0;

    return doConnect();
  }

  void disconnect() {
    shouldReconnect_ = false;
    shutdown_ = true;

    if (recvThread_.joinable()) {
      recvThread_.join();
    }

    cleanup();
  }

  bool sendText(const std::string &message) {
    if (!connected_)
      return false;

    std::lock_guard<std::mutex> lock(mutex_);
    DWORD err = WinHttpWebSocketSend(
        hWebSocket_, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
        const_cast<char *>(message.data()), static_cast<DWORD>(message.size()));
    return err == NO_ERROR;
  }

  bool sendBinary(const uint8_t *data, size_t len) {
    if (!connected_)
      return false;

    std::lock_guard<std::mutex> lock(mutex_);
    DWORD err = WinHttpWebSocketSend(
        hWebSocket_, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE,
        const_cast<uint8_t *>(data), static_cast<DWORD>(len));
    return err == NO_ERROR;
  }

  bool sendBinary(const std::vector<uint8_t> &data) {
    return sendBinary(data.data(), data.size());
  }

  bool pollMessage(WSMsg &msg) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (messages_.empty())
      return false;
    msg = std::move(messages_.front());
    messages_.pop();
    return true;
  }

  void onMessage(MessageHandler handler) { handler_ = std::move(handler); }

  bool isConnected() const { return connected_; }
  int reconnectAttempts() const { return attempts_; }
  const std::string &lastError() const { return lastError_; }

private:
  bool doConnect() {
    cleanup();

    std::string host, path;
    int port;
    bool secure;
    if (!parseUrl(url_, host, path, port, secure)) {
      lastError_ = "Invalid URL";
      return false;
    }

    hSession_ =
        WinHttpOpen(L"UniversalBridge/2.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                    nullptr, nullptr, 0);
    if (!hSession_) {
      lastError_ = "Session failed";
      return false;
    }

    std::wstring wHost(host.begin(), host.end());
    hConnect_ = WinHttpConnect(hSession_, wHost.c_str(),
                               static_cast<INTERNET_PORT>(port), 0);
    if (!hConnect_) {
      lastError_ = "Connect failed";
      cleanup();
      return false;
    }

    std::wstring wPath(path.begin(), path.end());
    DWORD flags = secure ? WINHTTP_FLAG_SECURE : 0;

    hRequest_ = WinHttpOpenRequest(hConnect_, L"GET", wPath.c_str(), nullptr,
                                   WINHTTP_NO_REFERER,
                                   WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest_) {
      lastError_ = "Request failed";
      cleanup();
      return false;
    }

    WinHttpSetOption(hRequest_, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr,
                     0);

    if (!WinHttpSendRequest(hRequest_, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest_, nullptr)) {
      lastError_ = "Handshake failed";
      cleanup();
      return false;
    }

    hWebSocket_ = WinHttpWebSocketCompleteUpgrade(hRequest_, 0);
    if (!hWebSocket_) {
      lastError_ = "Upgrade failed";
      cleanup();
      return false;
    }

    WinHttpCloseHandle(hRequest_);
    hRequest_ = nullptr;

    connected_ = true;
    shutdown_ = false;
    attempts_ = 0;

    recvThread_ = std::thread(&WebSocketAdv::recvLoop, this);

    return true;
  }

  void recvLoop() {
    std::vector<uint8_t> buffer(65536);

    while (!shutdown_) {
      if (!connected_) {
        if (shouldReconnect_ &&
            (policy_.maxAttempts == 0 || attempts_ < policy_.maxAttempts)) {

          int delay = policy_.calculateDelay(attempts_);
          attempts_++;

          std::this_thread::sleep_for(std::chrono::milliseconds(delay));

          std::lock_guard<std::mutex> lock(mutex_);
          if (doConnect())
            continue;
        } else {
          break;
        }
        continue;
      }

      DWORD bytesRead = 0;
      WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;

      DWORD err = WinHttpWebSocketReceive(hWebSocket_, buffer.data(),
                                          static_cast<DWORD>(buffer.size()),
                                          &bytesRead, &bufferType);

      if (err != NO_ERROR) {
        if (!shutdown_) {
          connected_ = false;
          pushError(static_cast<int>(err), "Receive error");
        }
        continue;
      }

      WSMsg msg;

      switch (bufferType) {
      case WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE:
      case WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE:
        msg.type = WSMsgType::TEXT;
        msg.data.assign(buffer.begin(), buffer.begin() + bytesRead);
        break;

      case WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE:
      case WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE:
        msg.type = WSMsgType::BINARY;
        msg.data.assign(buffer.begin(), buffer.begin() + bytesRead);
        break;

      case WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE:
        msg.type = WSMsgType::CLOSE;
        connected_ = false;
        break;

      default:
        continue;
      }

      pushMessage(msg);

      if (msg.type == WSMsgType::CLOSE)
        break;
    }
  }

  void pushMessage(const WSMsg &msg) {
    {
      std::lock_guard<std::mutex> lock(queueMutex_);
      messages_.push(msg);
    }
    if (handler_)
      handler_(msg);
  }

  void pushError(int code, const std::string &message) {
    WSMsg msg;
    msg.type = WSMsgType::ERR;
    msg.errCode = code;
    msg.errMessage = message;
    pushMessage(msg);
  }

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

  void cleanup() {
    connected_ = false;
    if (hWebSocket_) {
      WinHttpCloseHandle(hWebSocket_);
      hWebSocket_ = nullptr;
    }
    if (hRequest_) {
      WinHttpCloseHandle(hRequest_);
      hRequest_ = nullptr;
    }
    if (hConnect_) {
      WinHttpCloseHandle(hConnect_);
      hConnect_ = nullptr;
    }
    if (hSession_) {
      WinHttpCloseHandle(hSession_);
      hSession_ = nullptr;
    }
  }

  std::string url_;
  ReconnectPolicy policy_;

  HINTERNET hSession_ = nullptr;
  HINTERNET hConnect_ = nullptr;
  HINTERNET hRequest_ = nullptr;
  HINTERNET hWebSocket_ = nullptr;

  std::atomic<bool> connected_{false};
  std::atomic<bool> shutdown_{false};
  std::atomic<bool> shouldReconnect_{false};
  std::atomic<int> attempts_{0};

  std::thread recvThread_;
  std::mutex mutex_;
  std::mutex queueMutex_;
  std::queue<WSMsg> messages_;

  MessageHandler handler_;
  std::string lastError_;
};

} // namespace Hub::Network

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) void *hub_ws_adv_create();
__declspec(dllexport) int hub_ws_adv_connect(void *handle, const char *url,
                                             int auto_reconnect,
                                             int max_attempts);
__declspec(dllexport) void hub_ws_adv_disconnect(void *handle);
__declspec(dllexport) int hub_ws_adv_send_text(void *handle,
                                               const char *message);
__declspec(dllexport) int
hub_ws_adv_send_binary(void *handle, const unsigned char *data, int len);
__declspec(dllexport) int hub_ws_adv_poll_text(void *handle, char *buffer,
                                               int size);
__declspec(dllexport) int
hub_ws_adv_poll_binary(void *handle, unsigned char *buffer, int size);
__declspec(dllexport) int hub_ws_adv_is_connected(void *handle);
__declspec(dllexport) int hub_ws_adv_reconnect_attempts(void *handle);
__declspec(dllexport) void hub_ws_adv_destroy(void *handle);
}
