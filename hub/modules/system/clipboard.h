#pragma once
// =============================================================================
// CLIPBOARD.H - Clipboard Operations
// =============================================================================
// Clipboard access, shell commands, and notifications.
// Uses dynamic loading to avoid shellapi.h SDK conflicts.
// =============================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <string>

namespace Hub::System {

// =============================================================================
// CLIPBOARD MANAGER
// =============================================================================
class Clipboard {
public:
  // Copy text to clipboard
  static bool setText(const std::string &text) {
    if (!OpenClipboard(nullptr))
      return false;

    EmptyClipboard();

    size_t size = text.size() + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hMem) {
      CloseClipboard();
      return false;
    }

    char *pMem = static_cast<char *>(GlobalLock(hMem));
    if (pMem) {
      memcpy(pMem, text.c_str(), size);
      GlobalUnlock(hMem);
    }

    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();

    return true;
  }

  // Get text from clipboard
  static std::string getText() {
    if (!OpenClipboard(nullptr))
      return "";

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (!hData) {
      CloseClipboard();
      return "";
    }

    char *pText = static_cast<char *>(GlobalLock(hData));
    std::string text = pText ? pText : "";
    GlobalUnlock(hData);

    CloseClipboard();
    return text;
  }

  // Check if clipboard has text
  static bool hasText() { return IsClipboardFormatAvailable(CF_TEXT) != 0; }

  // Clear clipboard
  static bool clear() {
    if (!OpenClipboard(nullptr))
      return false;
    EmptyClipboard();
    CloseClipboard();
    return true;
  }
};

// =============================================================================
// SHELL COMMANDS (using dynamic loading to avoid SDK issues)
// =============================================================================
class Shell {
public:
  // Execute command and get output
  static std::string execute(const std::string &command) {
    std::string result;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
      return "Error: Failed to create pipe";
    }

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    std::string cmd = "cmd.exe /c " + command;

    if (!CreateProcessA(nullptr, const_cast<char *>(cmd.c_str()), nullptr,
                        nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si,
                        &pi)) {
      CloseHandle(hReadPipe);
      CloseHandle(hWritePipe);
      return "Error: Failed to create process";
    }

    CloseHandle(hWritePipe);

    char buffer[4096];
    DWORD bytesRead;
    while (
        ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) &&
        bytesRead > 0) {
      buffer[bytesRead] = '\0';
      result += buffer;
    }

    CloseHandle(hReadPipe);
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return result;
  }

  // Open URL in default browser (via cmd start)
  static bool openUrl(const std::string &url) {
    std::string cmd = "start \"\" \"" + url + "\"";
    Shell::execute(cmd);
    return true;
  }

  // Open file with default application
  static bool openFile(const std::string &path) {
    std::string cmd = "start \"\" \"" + path + "\"";
    Shell::execute(cmd);
    return true;
  }

  // Show file in explorer
  static bool showInExplorer(const std::string &path) {
    std::string cmd = "explorer /select,\"" + path + "\"";
    Shell::execute(cmd);
    return true;
  }
};

// =============================================================================
// NOTIFICATIONS (via PowerShell MessageBox)
// =============================================================================
class Notification {
public:
  static bool show(const std::string &title, const std::string &message) {
    // Use MessageBox API directly
    MessageBoxA(nullptr, message.c_str(), title.c_str(),
                MB_OK | MB_ICONINFORMATION);
    return true;
  }
};

} // namespace Hub::System

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_clipboard_set_text(const char *text);
__declspec(dllexport) const char *hub_clipboard_get_text();
__declspec(dllexport) int hub_clipboard_has_text();
__declspec(dllexport) int hub_clipboard_clear();

__declspec(dllexport) const char *hub_shell_execute(const char *command);
__declspec(dllexport) int hub_shell_open_url(const char *url);
__declspec(dllexport) int hub_shell_open_file(const char *path);
__declspec(dllexport) int hub_shell_show_in_explorer(const char *path);

__declspec(dllexport) int hub_notify_show(const char *title,
                                          const char *message);
}
