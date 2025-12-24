// =============================================================================
// BRIDGE_RUNTIME.CPP - Bridge Runtime Functions
// =============================================================================

#include "bridge.h"

// Global state for exceptions
std::string _bridge_error;
bool _bridge_has_error = false;

// Global callback queue
std::vector<std::string> _bridge_callback_queue;

// =============================================================================
// RUNTIME FUNCTIONS (exported)
// =============================================================================

BRIDGE str bridge_version() { return "2.0.0"; }

BRIDGE str bridge_features() {
  return RET("{\"features\":["
             "\"functions\","
             "\"variables\","
             "\"enums\","
             "\"structs\","
             "\"classes\","
             "\"methods\","
             "\"exceptions\","
             "\"callbacks\""
             "]}");
}

BRIDGE str bridge_get_last_error() {
  if (_bridge_has_error) {
    _bridge_has_error = false;
    return _bridge_error.c_str();
  }
  return "";
}

BRIDGE int bridge_has_error() { return _bridge_has_error ? 1 : 0; }

BRIDGE str bridge_poll_callbacks() {
  if (_bridge_callback_queue.empty())
    return "[]";
  std::ostringstream ss;
  ss << "[";
  for (size_t i = 0; i < _bridge_callback_queue.size(); i++) {
    if (i > 0)
      ss << ",";
    ss << _bridge_callback_queue[i];
  }
  ss << "]";
  _bridge_callback_queue.clear();
  return RET(ss.str());
}
