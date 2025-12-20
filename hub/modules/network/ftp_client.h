#pragma once
// =============================================================================
// FTP_CLIENT.H - FTP Client Module
// =============================================================================
// File transfer via FTP/FTPS using Windows WinInet API.
// Supports upload, download, list, delete operations.
// =============================================================================

#include <atomic>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <windows.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

namespace Hub::Network {

// =============================================================================
// FTP FILE INFO
// =============================================================================
struct FtpFileInfo {
  std::string name;
  uint64_t size = 0;
  bool is_directory = false;
  std::string modified_time;
};

// =============================================================================
// FTP CLIENT
// =============================================================================
class FtpClient {
public:
  FtpClient() = default;
  ~FtpClient() { disconnect(); }

  // Connect to FTP server
  bool connect(const std::string &host, const std::string &username,
               const std::string &password, int port = 21,
               bool passive = true) {
    if (connected_)
      disconnect();

    std::cout << "[ftp] Connecting to: " << host << ":" << port << "\n";

    hInternet_ = InternetOpenA("UniversalBridge/2.0", INTERNET_OPEN_TYPE_DIRECT,
                               NULL, NULL, 0);
    if (!hInternet_) {
      last_error_ = "InternetOpen failed: " + std::to_string(GetLastError());
      return false;
    }

    DWORD flags = INTERNET_FLAG_PASSIVE;
    if (!passive)
      flags = 0;

    hFtp_ = InternetConnectA(hInternet_, host.c_str(),
                             static_cast<INTERNET_PORT>(port), username.c_str(),
                             password.c_str(), INTERNET_SERVICE_FTP, flags, 0);
    if (!hFtp_) {
      last_error_ = "FTP connect failed: " + std::to_string(GetLastError());
      InternetCloseHandle(hInternet_);
      hInternet_ = NULL;
      return false;
    }

    connected_ = true;
    host_ = host;
    std::cout << "[ftp] Connected successfully\n";
    return true;
  }

  void disconnect() {
    if (hFtp_) {
      InternetCloseHandle(hFtp_);
      hFtp_ = NULL;
    }
    if (hInternet_) {
      InternetCloseHandle(hInternet_);
      hInternet_ = NULL;
    }
    connected_ = false;
  }

  // Download file
  bool downloadFile(const std::string &remotePath,
                    const std::string &localPath) {
    if (!connected_) {
      last_error_ = "Not connected";
      return false;
    }

    std::cout << "[ftp] Downloading: " << remotePath << " -> " << localPath
              << "\n";

    if (FtpGetFileA(hFtp_, remotePath.c_str(), localPath.c_str(), FALSE,
                    FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0)) {
      std::cout << "[ftp] Download complete\n";
      return true;
    }

    last_error_ = "Download failed: " + std::to_string(GetLastError());
    return false;
  }

  // Upload file
  bool uploadFile(const std::string &localPath, const std::string &remotePath) {
    if (!connected_) {
      last_error_ = "Not connected";
      return false;
    }

    std::cout << "[ftp] Uploading: " << localPath << " -> " << remotePath
              << "\n";

    if (FtpPutFileA(hFtp_, localPath.c_str(), remotePath.c_str(),
                    FTP_TRANSFER_TYPE_BINARY, 0)) {
      std::cout << "[ftp] Upload complete\n";
      return true;
    }

    last_error_ = "Upload failed: " + std::to_string(GetLastError());
    return false;
  }

  // Delete remote file
  bool deleteFile(const std::string &remotePath) {
    if (!connected_) {
      last_error_ = "Not connected";
      return false;
    }

    if (FtpDeleteFileA(hFtp_, remotePath.c_str())) {
      std::cout << "[ftp] Deleted: " << remotePath << "\n";
      return true;
    }

    last_error_ = "Delete failed: " + std::to_string(GetLastError());
    return false;
  }

  // Create directory
  bool createDirectory(const std::string &remotePath) {
    if (!connected_) {
      last_error_ = "Not connected";
      return false;
    }

    if (FtpCreateDirectoryA(hFtp_, remotePath.c_str())) {
      return true;
    }

    last_error_ = "Create directory failed: " + std::to_string(GetLastError());
    return false;
  }

  // Remove directory
  bool removeDirectory(const std::string &remotePath) {
    if (!connected_) {
      last_error_ = "Not connected";
      return false;
    }

    if (FtpRemoveDirectoryA(hFtp_, remotePath.c_str())) {
      return true;
    }

    last_error_ = "Remove directory failed: " + std::to_string(GetLastError());
    return false;
  }

  // List directory contents
  std::vector<FtpFileInfo> listDirectory(const std::string &remotePath = "") {
    std::vector<FtpFileInfo> files;

    if (!connected_) {
      last_error_ = "Not connected";
      return files;
    }

    WIN32_FIND_DATAA findData;
    std::string searchPath = remotePath.empty() ? "*" : remotePath + "/*";

    HINTERNET hFind =
        FtpFindFirstFileA(hFtp_, searchPath.c_str(), &findData, 0, 0);
    if (!hFind) {
      if (GetLastError() != ERROR_NO_MORE_FILES) {
        last_error_ = "List failed: " + std::to_string(GetLastError());
      }
      return files;
    }

    do {
      FtpFileInfo info;
      info.name = findData.cFileName;
      info.is_directory =
          (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
      info.size = (static_cast<uint64_t>(findData.nFileSizeHigh) << 32) |
                  findData.nFileSizeLow;
      files.push_back(info);
    } while (InternetFindNextFileA(hFind, &findData));

    InternetCloseHandle(hFind);
    return files;
  }

  // Change current directory
  bool setCurrentDirectory(const std::string &remotePath) {
    if (!connected_) {
      last_error_ = "Not connected";
      return false;
    }

    if (FtpSetCurrentDirectoryA(hFtp_, remotePath.c_str())) {
      return true;
    }

    last_error_ = "CD failed: " + std::to_string(GetLastError());
    return false;
  }

  // Get current directory
  std::string getCurrentDirectory() {
    if (!connected_)
      return "";

    char buffer[MAX_PATH];
    DWORD bufSize = MAX_PATH;

    if (FtpGetCurrentDirectoryA(hFtp_, buffer, &bufSize)) {
      return std::string(buffer);
    }

    return "";
  }

  bool isConnected() const { return connected_; }
  const std::string &getLastError() const { return last_error_; }

private:
  HINTERNET hInternet_ = NULL;
  HINTERNET hFtp_ = NULL;
  bool connected_ = false;
  std::string host_;
  std::string last_error_;
};

// Storage for FTP clients
inline std::mutex ftpClientMutex;
inline std::map<int, std::unique_ptr<FtpClient>> ftpClients;
inline std::atomic<int> ftpNextHandle{1};

inline int createFtpClient() {
  std::lock_guard<std::mutex> lock(ftpClientMutex);
  int handle = ftpNextHandle++;
  ftpClients[handle] = std::make_unique<FtpClient>();
  return handle;
}

inline FtpClient *getFtpClient(int handle) {
  std::lock_guard<std::mutex> lock(ftpClientMutex);
  auto it = ftpClients.find(handle);
  return (it != ftpClients.end()) ? it->second.get() : nullptr;
}

inline void releaseFtpClient(int handle) {
  std::lock_guard<std::mutex> lock(ftpClientMutex);
  ftpClients.erase(handle);
}

} // namespace Hub::Network

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_ftp_create();
__declspec(dllexport) int hub_ftp_connect(int handle, const char *host,
                                          const char *user, const char *pass,
                                          int port);
__declspec(dllexport) int hub_ftp_download(int handle, const char *remote,
                                           const char *local);
__declspec(dllexport) int hub_ftp_upload(int handle, const char *local,
                                         const char *remote);
__declspec(dllexport) int hub_ftp_delete(int handle, const char *remote);
__declspec(dllexport) int hub_ftp_mkdir(int handle, const char *path);
__declspec(dllexport) void hub_ftp_disconnect(int handle);
__declspec(dllexport) const char *hub_ftp_get_error(int handle);
}
