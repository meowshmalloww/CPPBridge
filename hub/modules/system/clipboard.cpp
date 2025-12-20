// =============================================================================
// CLIPBOARD.CPP - Clipboard Implementation
// =============================================================================

#include "clipboard.h"

static thread_local std::string tl_clipboard_text;
static thread_local std::string tl_shell_output;

extern "C" {

__declspec(dllexport) int hub_clipboard_set_text(const char *text) {
  if (!text)
    return 0;
  return Hub::System::Clipboard::setText(text) ? 1 : 0;
}

__declspec(dllexport) const char *hub_clipboard_get_text() {
  tl_clipboard_text = Hub::System::Clipboard::getText();
  return tl_clipboard_text.c_str();
}

__declspec(dllexport) int hub_clipboard_has_text() {
  return Hub::System::Clipboard::hasText() ? 1 : 0;
}

__declspec(dllexport) int hub_clipboard_clear() {
  return Hub::System::Clipboard::clear() ? 1 : 0;
}

__declspec(dllexport) const char *hub_shell_execute(const char *command) {
  if (!command)
    return "";
  tl_shell_output = Hub::System::Shell::execute(command);
  return tl_shell_output.c_str();
}

__declspec(dllexport) int hub_shell_open_url(const char *url) {
  if (!url)
    return 0;
  return Hub::System::Shell::openUrl(url) ? 1 : 0;
}

__declspec(dllexport) int hub_shell_open_file(const char *path) {
  if (!path)
    return 0;
  return Hub::System::Shell::openFile(path) ? 1 : 0;
}

__declspec(dllexport) int hub_shell_show_in_explorer(const char *path) {
  if (!path)
    return 0;
  return Hub::System::Shell::showInExplorer(path) ? 1 : 0;
}

__declspec(dllexport) int hub_notify_show(const char *title,
                                          const char *message) {
  if (!title || !message)
    return 0;
  return Hub::System::Notification::show(title, message) ? 1 : 0;
}

} // extern "C"
