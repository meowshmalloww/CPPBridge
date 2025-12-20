// =============================================================================
// FTP_CLIENT.CPP - FTP Client Implementation
// =============================================================================

#include "ftp_client.h"
#include <atomic>
#include <map>
#include <memory>
#include <mutex>


// Thread-local error storage
static thread_local std::string tl_last_error;

extern "C" {

__declspec(dllexport) int hub_ftp_create() {
  return Hub::Network::createFtpClient();
}

__declspec(dllexport) int hub_ftp_connect(int handle, const char *host,
                                          const char *user, const char *pass,
                                          int port) {
  if (!host || !user || !pass) {
    tl_last_error = "Invalid arguments";
    return -1;
  }

  auto *client = Hub::Network::getFtpClient(handle);
  if (!client) {
    tl_last_error = "Invalid handle";
    return -1;
  }

  if (client->connect(host, user, pass, port > 0 ? port : 21)) {
    return 0;
  }

  tl_last_error = client->getLastError();
  return -1;
}

__declspec(dllexport) int hub_ftp_download(int handle, const char *remote,
                                           const char *local) {
  if (!remote || !local) {
    tl_last_error = "Invalid arguments";
    return -1;
  }

  auto *client = Hub::Network::getFtpClient(handle);
  if (!client) {
    tl_last_error = "Invalid handle";
    return -1;
  }

  if (client->downloadFile(remote, local)) {
    return 0;
  }

  tl_last_error = client->getLastError();
  return -1;
}

__declspec(dllexport) int hub_ftp_upload(int handle, const char *local,
                                         const char *remote) {
  if (!local || !remote) {
    tl_last_error = "Invalid arguments";
    return -1;
  }

  auto *client = Hub::Network::getFtpClient(handle);
  if (!client) {
    tl_last_error = "Invalid handle";
    return -1;
  }

  if (client->uploadFile(local, remote)) {
    return 0;
  }

  tl_last_error = client->getLastError();
  return -1;
}

__declspec(dllexport) int hub_ftp_delete(int handle, const char *remote) {
  if (!remote) {
    tl_last_error = "Invalid arguments";
    return -1;
  }

  auto *client = Hub::Network::getFtpClient(handle);
  if (!client) {
    tl_last_error = "Invalid handle";
    return -1;
  }

  if (client->deleteFile(remote)) {
    return 0;
  }

  tl_last_error = client->getLastError();
  return -1;
}

__declspec(dllexport) int hub_ftp_mkdir(int handle, const char *path) {
  if (!path) {
    tl_last_error = "Invalid arguments";
    return -1;
  }

  auto *client = Hub::Network::getFtpClient(handle);
  if (!client) {
    tl_last_error = "Invalid handle";
    return -1;
  }

  if (client->createDirectory(path)) {
    return 0;
  }

  tl_last_error = client->getLastError();
  return -1;
}

__declspec(dllexport) void hub_ftp_disconnect(int handle) {
  auto *client = Hub::Network::getFtpClient(handle);
  if (client) {
    client->disconnect();
  }
  Hub::Network::releaseFtpClient(handle);
}

__declspec(dllexport) const char *hub_ftp_get_error(int handle) {
  auto *client = Hub::Network::getFtpClient(handle);
  if (client) {
    tl_last_error = client->getLastError();
  }
  return tl_last_error.c_str();
}

} // extern "C"
