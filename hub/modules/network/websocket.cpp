// =============================================================================
// WEBSOCKET.CPP - WebSocket Client Implementation
// =============================================================================

#include "websocket.h"
#include <cstring>

// Thread-local error storage
static thread_local std::string tl_last_error;

extern "C" {

__declspec(dllexport) int hub_ws_create() {
  return Hub::Network::createWebSocketClient();
}

__declspec(dllexport) int hub_ws_connect(int handle, const char *url) {
  if (!url) {
    tl_last_error = "URL is null";
    return -1;
  }

  auto *client = Hub::Network::getWebSocketClient(handle);
  if (!client) {
    tl_last_error = "Invalid handle";
    return -1;
  }

  if (client->connect(url)) {
    return 0; // Success
  }

  tl_last_error = client->getLastError();
  return -1;
}

__declspec(dllexport) int hub_ws_send(int handle, const char *message) {
  if (!message) {
    tl_last_error = "Message is null";
    return -1;
  }

  auto *client = Hub::Network::getWebSocketClient(handle);
  if (!client) {
    tl_last_error = "Invalid handle";
    return -1;
  }

  if (client->send(message)) {
    return 0;
  }

  tl_last_error = client->getLastError();
  return -1;
}

__declspec(dllexport) int hub_ws_poll(int handle, char *buffer,
                                      int buffer_size) {
  auto *client = Hub::Network::getWebSocketClient(handle);
  if (!client)
    return -1;

  Hub::Network::WebSocketMessage msg;
  if (!client->pollMessage(msg)) {
    return 0; // No message available
  }

  if (!msg.is_ok()) {
    tl_last_error = msg.error_message;
    return -1;
  }

  if (buffer && buffer_size > 0) {
    size_t copy_size =
        (std::min)(static_cast<size_t>(buffer_size - 1), msg.data.size());
    std::memcpy(buffer, msg.data.data(), copy_size);
    buffer[copy_size] = '\0';
  }

  // Return message type + 1 (so 0 means no message)
  return static_cast<int>(msg.type) + 1;
}

__declspec(dllexport) int hub_ws_is_connected(int handle) {
  auto *client = Hub::Network::getWebSocketClient(handle);
  if (!client)
    return 0;
  return client->isConnected() ? 1 : 0;
}

__declspec(dllexport) void hub_ws_close(int handle) {
  auto *client = Hub::Network::getWebSocketClient(handle);
  if (client) {
    client->close();
  }
  Hub::Network::releaseWebSocketClient(handle);
}

__declspec(dllexport) const char *hub_ws_get_error(int handle) {
  auto *client = Hub::Network::getWebSocketClient(handle);
  if (client) {
    tl_last_error = client->getLastError();
  }
  return tl_last_error.c_str();
}

} // extern "C"
