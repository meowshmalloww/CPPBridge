// =============================================================================
// WSSERVER.CPP - WebSocket Server Implementation
// =============================================================================

#include "wsserver.h"

extern "C" {

__declspec(dllexport) int hub_ws_server_start(int port) {
  return Hub::Network::globalWSServer().start(static_cast<uint16_t>(port)) ? 1
                                                                           : 0;
}

__declspec(dllexport) void hub_ws_server_stop() {
  Hub::Network::globalWSServer().stop();
}

__declspec(dllexport) int hub_ws_server_running() {
  return Hub::Network::globalWSServer().isRunning() ? 1 : 0;
}

__declspec(dllexport) int hub_ws_server_client_count() {
  return static_cast<int>(Hub::Network::globalWSServer().clientCount());
}

__declspec(dllexport) int hub_ws_server_send(unsigned int client_id,
                                             const char *message) {
  if (!message)
    return 0;
  return Hub::Network::globalWSServer().send(client_id, message) ? 1 : 0;
}

__declspec(dllexport) void hub_ws_server_broadcast(const char *message) {
  if (!message)
    return;
  Hub::Network::globalWSServer().broadcast(message);
}

__declspec(dllexport) void hub_ws_server_close_client(unsigned int client_id) {
  Hub::Network::globalWSServer().closeClient(client_id);
}

} // extern "C"
