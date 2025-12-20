// =============================================================================
// WEBSOCKET_ADV.CPP - Advanced WebSocket Implementation
// =============================================================================

#include "websocket_adv.h"
#include <cstring>

extern "C" {

__declspec(dllexport) void *hub_ws_adv_create() {
  try {
    return new Hub::Network::WebSocketAdv();
  } catch (...) {
    return nullptr;
  }
}

__declspec(dllexport) int hub_ws_adv_connect(void *handle, const char *url,
                                             int auto_reconnect,
                                             int max_attempts) {
  if (!handle || !url)
    return 0;

  try {
    auto *ws = static_cast<Hub::Network::WebSocketAdv *>(handle);
    Hub::Network::ReconnectPolicy policy;
    policy.enabled = (auto_reconnect != 0);
    policy.maxAttempts = max_attempts;
    return ws->connect(url, policy) ? 1 : 0;
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) void hub_ws_adv_disconnect(void *handle) {
  if (!handle)
    return;
  try {
    auto *ws = static_cast<Hub::Network::WebSocketAdv *>(handle);
    ws->disconnect();
  } catch (...) {
  }
}

__declspec(dllexport) int hub_ws_adv_send_text(void *handle,
                                               const char *message) {
  if (!handle || !message)
    return 0;
  try {
    auto *ws = static_cast<Hub::Network::WebSocketAdv *>(handle);
    return ws->sendText(message) ? 1 : 0;
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) int
hub_ws_adv_send_binary(void *handle, const unsigned char *data, int len) {
  if (!handle || !data || len <= 0)
    return 0;
  try {
    auto *ws = static_cast<Hub::Network::WebSocketAdv *>(handle);
    return ws->sendBinary(data, static_cast<size_t>(len)) ? 1 : 0;
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) int hub_ws_adv_poll_text(void *handle, char *buffer,
                                               int size) {
  if (!handle || !buffer || size <= 0)
    return 0;
  try {
    auto *ws = static_cast<Hub::Network::WebSocketAdv *>(handle);
    Hub::Network::WSMsg msg;
    if (!ws->pollMessage(msg))
      return 0;
    if (msg.type != Hub::Network::WSMsgType::TEXT)
      return 0;

    int len = static_cast<int>(msg.data.size());
    if (len >= size)
      len = size - 1;
    std::memcpy(buffer, msg.data.data(), len);
    buffer[len] = '\0';
    return len;
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) int
hub_ws_adv_poll_binary(void *handle, unsigned char *buffer, int size) {
  if (!handle || !buffer || size <= 0)
    return 0;
  try {
    auto *ws = static_cast<Hub::Network::WebSocketAdv *>(handle);
    Hub::Network::WSMsg msg;
    if (!ws->pollMessage(msg))
      return 0;
    if (msg.type != Hub::Network::WSMsgType::BINARY)
      return 0;

    int len = static_cast<int>(msg.data.size());
    if (len > size)
      len = size;
    std::memcpy(buffer, msg.data.data(), len);
    return len;
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) int hub_ws_adv_is_connected(void *handle) {
  if (!handle)
    return 0;
  try {
    auto *ws = static_cast<Hub::Network::WebSocketAdv *>(handle);
    return ws->isConnected() ? 1 : 0;
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) int hub_ws_adv_reconnect_attempts(void *handle) {
  if (!handle)
    return 0;
  try {
    auto *ws = static_cast<Hub::Network::WebSocketAdv *>(handle);
    return ws->reconnectAttempts();
  } catch (...) {
    return 0;
  }
}

__declspec(dllexport) void hub_ws_adv_destroy(void *handle) {
  if (!handle)
    return;
  try {
    auto *ws = static_cast<Hub::Network::WebSocketAdv *>(handle);
    delete ws;
  } catch (...) {
  }
}

} // extern "C"
