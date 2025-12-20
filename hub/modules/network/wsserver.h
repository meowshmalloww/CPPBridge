#pragma once
// =============================================================================
// WSSERVER.H - WebSocket Server (RFC 6455)
// =============================================================================
// Complete WebSocket server implementation:
// - TCP socket listener
// - HTTP upgrade handshake
// - Frame encoding/decoding
// - Client management
// - Ping/pong heartbeat
// =============================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace Hub::Network {

// =============================================================================
// WEBSOCKET OPCODES
// =============================================================================
enum class WSOpcode : uint8_t {
  CONTINUATION = 0x0,
  TEXT = 0x1,
  BINARY = 0x2,
  CLOSE = 0x8,
  PING = 0x9,
  PONG = 0xA
};

// =============================================================================
// WEBSOCKET CLIENT
// =============================================================================
struct WSClient {
  uint32_t id;
  SOCKET socket;
  std::string remoteAddr;
  uint16_t remotePort;
  bool connected;
  std::chrono::steady_clock::time_point lastActivity;
};

// =============================================================================
// WEBSOCKET MESSAGE
// =============================================================================
struct WSMessage {
  uint32_t clientId;
  WSOpcode opcode;
  std::vector<uint8_t> data;

  std::string text() const { return std::string(data.begin(), data.end()); }
};

// =============================================================================
// WEBSOCKET SERVER
// =============================================================================
class WebSocketServer {
public:
  using MessageCallback = std::function<void(const WSMessage &)>;
  using ConnectCallback = std::function<void(uint32_t clientId)>;
  using DisconnectCallback = std::function<void(uint32_t clientId)>;

  WebSocketServer() = default;
  ~WebSocketServer() { stop(); }

  // Start server on port
  bool start(uint16_t port) {
    if (running_)
      return false;

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
      return false;
    }

    // Create socket
    listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket_ == INVALID_SOCKET) {
      WSACleanup();
      return false;
    }

    // Allow reuse
    int opt = 1;
    setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char *>(&opt), sizeof(opt));

    // Bind
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listenSocket_, reinterpret_cast<sockaddr *>(&addr),
             sizeof(addr)) == SOCKET_ERROR) {
      closesocket(listenSocket_);
      WSACleanup();
      return false;
    }

    // Listen
    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) {
      closesocket(listenSocket_);
      WSACleanup();
      return false;
    }

    port_ = port;
    running_ = true;

    // Start accept thread
    acceptThread_ = std::thread(&WebSocketServer::acceptLoop, this);

    return true;
  }

  // Stop server
  void stop() {
    if (!running_)
      return;

    running_ = false;

    // Close listen socket to unblock accept
    if (listenSocket_ != INVALID_SOCKET) {
      closesocket(listenSocket_);
      listenSocket_ = INVALID_SOCKET;
    }

    // Close all clients
    {
      std::lock_guard<std::mutex> lock(clientsMutex_);
      for (auto &[id, client] : clients_) {
        if (client.socket != INVALID_SOCKET) {
          closesocket(client.socket);
        }
      }
      clients_.clear();
    }

    // Wait for threads
    if (acceptThread_.joinable()) {
      acceptThread_.join();
    }

    for (auto &t : clientThreads_) {
      if (t.joinable())
        t.join();
    }
    clientThreads_.clear();

    WSACleanup();
  }

  // Send message to client
  bool send(uint32_t clientId, const std::string &message) {
    return sendFrame(clientId, WSOpcode::TEXT,
                     std::vector<uint8_t>(message.begin(), message.end()));
  }

  bool sendBinary(uint32_t clientId, const std::vector<uint8_t> &data) {
    return sendFrame(clientId, WSOpcode::BINARY, data);
  }

  // Broadcast to all clients
  void broadcast(const std::string &message) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    for (auto &[id, client] : clients_) {
      if (client.connected) {
        sendFrame(id, WSOpcode::TEXT,
                  std::vector<uint8_t>(message.begin(), message.end()));
      }
    }
  }

  // Close client connection
  void closeClient(uint32_t clientId) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(clientId);
    if (it != clients_.end()) {
      sendFrame(clientId, WSOpcode::CLOSE, {});
      closesocket(it->second.socket);
      it->second.connected = false;
    }
  }

  // Set callbacks
  void onMessage(MessageCallback cb) { onMessage_ = std::move(cb); }
  void onConnect(ConnectCallback cb) { onConnect_ = std::move(cb); }
  void onDisconnect(DisconnectCallback cb) { onDisconnect_ = std::move(cb); }

  // Getters
  bool isRunning() const { return running_; }
  uint16_t port() const { return port_; }
  size_t clientCount() const {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    return clients_.size();
  }

private:
  void acceptLoop() {
    while (running_) {
      sockaddr_in clientAddr;
      int addrLen = sizeof(clientAddr);

      SOCKET clientSocket = accept(
          listenSocket_, reinterpret_cast<sockaddr *>(&clientAddr), &addrLen);

      if (clientSocket == INVALID_SOCKET) {
        if (running_)
          continue;
        break;
      }

      // Create client
      WSClient client;
      client.id = nextClientId_++;
      client.socket = clientSocket;
      client.connected = false;
      client.lastActivity = std::chrono::steady_clock::now();

      char addrStr[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &clientAddr.sin_addr, addrStr, sizeof(addrStr));
      client.remoteAddr = addrStr;
      client.remotePort = ntohs(clientAddr.sin_port);

      {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        clients_[client.id] = client;
      }

      // Start client thread
      clientThreads_.emplace_back(&WebSocketServer::clientLoop, this,
                                  client.id);
    }
  }

  void clientLoop(uint32_t clientId) {
    WSClient *client = nullptr;
    {
      std::lock_guard<std::mutex> lock(clientsMutex_);
      auto it = clients_.find(clientId);
      if (it == clients_.end())
        return;
      client = &it->second;
    }

    // Perform WebSocket handshake
    if (!performHandshake(client->socket)) {
      closesocket(client->socket);
      std::lock_guard<std::mutex> lock(clientsMutex_);
      clients_.erase(clientId);
      return;
    }

    client->connected = true;

    // Notify connection
    if (onConnect_)
      onConnect_(clientId);

    // Read loop
    while (running_ && client->connected) {
      WSMessage msg;
      if (readFrame(client->socket, msg)) {
        msg.clientId = clientId;
        client->lastActivity = std::chrono::steady_clock::now();

        switch (msg.opcode) {
        case WSOpcode::TEXT:
        case WSOpcode::BINARY:
          if (onMessage_)
            onMessage_(msg);
          break;
        case WSOpcode::PING:
          sendFrame(clientId, WSOpcode::PONG, msg.data);
          break;
        case WSOpcode::CLOSE:
          client->connected = false;
          break;
        default:
          break;
        }
      } else {
        client->connected = false;
      }
    }

    // Notify disconnection
    if (onDisconnect_)
      onDisconnect_(clientId);

    closesocket(client->socket);

    std::lock_guard<std::mutex> lock(clientsMutex_);
    clients_.erase(clientId);
  }

  bool performHandshake(SOCKET sock) {
    char buffer[4096];
    int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0)
      return false;
    buffer[received] = '\0';

    std::string request(buffer);

    // Find Sec-WebSocket-Key
    size_t keyPos = request.find("Sec-WebSocket-Key: ");
    if (keyPos == std::string::npos)
      return false;

    size_t keyEnd = request.find("\r\n", keyPos);
    std::string key = request.substr(keyPos + 19, keyEnd - keyPos - 19);

    // Generate accept key
    std::string acceptKey = generateAcceptKey(key);

    // Send response
    std::string response = "HTTP/1.1 101 Switching Protocols\r\n"
                           "Upgrade: websocket\r\n"
                           "Connection: Upgrade\r\n"
                           "Sec-WebSocket-Accept: " +
                           acceptKey + "\r\n\r\n";

    return ::send(sock, response.c_str(), static_cast<int>(response.size()),
                  0) > 0;
  }

  std::string generateAcceptKey(const std::string &clientKey) {
    // Append magic GUID
    std::string combined = clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    // SHA-1 hash
    std::vector<uint8_t> hash = sha1(combined);

    // Base64 encode
    return base64Encode(hash);
  }

  std::vector<uint8_t> sha1(const std::string &input) {
    // Simple SHA-1 implementation (for WebSocket handshake only)
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    std::vector<uint8_t> msg(input.begin(), input.end());
    uint64_t bitLen = msg.size() * 8;

    // Padding
    msg.push_back(0x80);
    while ((msg.size() % 64) != 56) {
      msg.push_back(0x00);
    }

    // Append length
    for (int i = 7; i >= 0; --i) {
      msg.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF));
    }

    // Process blocks
    for (size_t i = 0; i < msg.size(); i += 64) {
      uint32_t w[80];
      for (int j = 0; j < 16; ++j) {
        w[j] = (msg[i + j * 4] << 24) | (msg[i + j * 4 + 1] << 16) |
               (msg[i + j * 4 + 2] << 8) | msg[i + j * 4 + 3];
      }
      for (int j = 16; j < 80; ++j) {
        uint32_t t = w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16];
        w[j] = (t << 1) | (t >> 31);
      }

      uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;

      for (int j = 0; j < 80; ++j) {
        uint32_t f, k;
        if (j < 20) {
          f = (b & c) | ((~b) & d);
          k = 0x5A827999;
        } else if (j < 40) {
          f = b ^ c ^ d;
          k = 0x6ED9EBA1;
        } else if (j < 60) {
          f = (b & c) | (b & d) | (c & d);
          k = 0x8F1BBCDC;
        } else {
          f = b ^ c ^ d;
          k = 0xCA62C1D6;
        }

        uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[j];
        e = d;
        d = c;
        c = (b << 30) | (b >> 2);
        b = a;
        a = temp;
      }

      h0 += a;
      h1 += b;
      h2 += c;
      h3 += d;
      h4 += e;
    }

    std::vector<uint8_t> result(20);
    for (int i = 0; i < 4; ++i) {
      result[i] = (h0 >> (24 - i * 8)) & 0xFF;
      result[4 + i] = (h1 >> (24 - i * 8)) & 0xFF;
      result[8 + i] = (h2 >> (24 - i * 8)) & 0xFF;
      result[12 + i] = (h3 >> (24 - i * 8)) & 0xFF;
      result[16 + i] = (h4 >> (24 - i * 8)) & 0xFF;
    }

    return result;
  }

  std::string base64Encode(const std::vector<uint8_t> &data) {
    static const char *chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    result.reserve((data.size() + 2) / 3 * 4);

    for (size_t i = 0; i < data.size(); i += 3) {
      uint32_t n = static_cast<uint32_t>(data[i]) << 16;
      if (i + 1 < data.size())
        n |= static_cast<uint32_t>(data[i + 1]) << 8;
      if (i + 2 < data.size())
        n |= data[i + 2];

      result += chars[(n >> 18) & 0x3F];
      result += chars[(n >> 12) & 0x3F];
      result += (i + 1 < data.size()) ? chars[(n >> 6) & 0x3F] : '=';
      result += (i + 2 < data.size()) ? chars[n & 0x3F] : '=';
    }

    return result;
  }

  bool readFrame(SOCKET sock, WSMessage &msg) {
    uint8_t header[2];
    if (recv(sock, reinterpret_cast<char *>(header), 2, MSG_WAITALL) != 2) {
      return false;
    }

    bool fin = (header[0] & 0x80) != 0;
    msg.opcode = static_cast<WSOpcode>(header[0] & 0x0F);
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payloadLen = header[1] & 0x7F;

    // Extended payload length
    if (payloadLen == 126) {
      uint8_t ext[2];
      if (recv(sock, reinterpret_cast<char *>(ext), 2, MSG_WAITALL) != 2) {
        return false;
      }
      payloadLen = (ext[0] << 8) | ext[1];
    } else if (payloadLen == 127) {
      uint8_t ext[8];
      if (recv(sock, reinterpret_cast<char *>(ext), 8, MSG_WAITALL) != 8) {
        return false;
      }
      payloadLen = 0;
      for (int i = 0; i < 8; ++i) {
        payloadLen = (payloadLen << 8) | ext[i];
      }
    }

    // Masking key
    uint8_t mask[4] = {0};
    if (masked) {
      if (recv(sock, reinterpret_cast<char *>(mask), 4, MSG_WAITALL) != 4) {
        return false;
      }
    }

    // Payload
    msg.data.resize(payloadLen);
    if (payloadLen > 0) {
      int received = recv(sock, reinterpret_cast<char *>(msg.data.data()),
                          static_cast<int>(payloadLen), MSG_WAITALL);
      if (received != static_cast<int>(payloadLen)) {
        return false;
      }

      // Unmask
      if (masked) {
        for (size_t i = 0; i < msg.data.size(); ++i) {
          msg.data[i] ^= mask[i % 4];
        }
      }
    }

    (void)fin; // For now, assume single-frame messages
    return true;
  }

  bool sendFrame(uint32_t clientId, WSOpcode opcode,
                 const std::vector<uint8_t> &data) {
    SOCKET sock = INVALID_SOCKET;
    {
      std::lock_guard<std::mutex> lock(clientsMutex_);
      auto it = clients_.find(clientId);
      if (it == clients_.end())
        return false;
      sock = it->second.socket;
    }

    std::vector<uint8_t> frame;

    // Header
    frame.push_back(0x80 | static_cast<uint8_t>(opcode)); // FIN + opcode

    // Payload length (server doesn't mask)
    if (data.size() <= 125) {
      frame.push_back(static_cast<uint8_t>(data.size()));
    } else if (data.size() <= 65535) {
      frame.push_back(126);
      frame.push_back((data.size() >> 8) & 0xFF);
      frame.push_back(data.size() & 0xFF);
    } else {
      frame.push_back(127);
      for (int i = 7; i >= 0; --i) {
        frame.push_back((data.size() >> (i * 8)) & 0xFF);
      }
    }

    // Payload
    frame.insert(frame.end(), data.begin(), data.end());

    return ::send(sock, reinterpret_cast<const char *>(frame.data()),
                  static_cast<int>(frame.size()), 0) > 0;
  }

  SOCKET listenSocket_ = INVALID_SOCKET;
  uint16_t port_ = 0;
  std::atomic<bool> running_{false};
  std::atomic<uint32_t> nextClientId_{1};

  mutable std::mutex clientsMutex_;
  std::map<uint32_t, WSClient> clients_;

  std::thread acceptThread_;
  std::vector<std::thread> clientThreads_;

  MessageCallback onMessage_;
  ConnectCallback onConnect_;
  DisconnectCallback onDisconnect_;
};

// =============================================================================
// GLOBAL INSTANCE
// =============================================================================
inline WebSocketServer &globalWSServer() {
  static WebSocketServer server;
  return server;
}

} // namespace Hub::Network

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_ws_server_start(int port);
__declspec(dllexport) void hub_ws_server_stop();
__declspec(dllexport) int hub_ws_server_running();
__declspec(dllexport) int hub_ws_server_client_count();
__declspec(dllexport) int hub_ws_server_send(unsigned int client_id,
                                             const char *message);
__declspec(dllexport) void hub_ws_server_broadcast(const char *message);
__declspec(dllexport) void hub_ws_server_close_client(unsigned int client_id);
}
