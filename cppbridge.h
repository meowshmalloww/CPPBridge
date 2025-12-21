// =============================================================================
// CPPBRIDGE.H - ONE MAGIC HEADER (Include this and you're done!)
// =============================================================================
//
// HOW TO USE:
// 1. Clone this repo into your project
// 2. Create any .cpp file anywhere
// 3. Add this ONE LINE at the top:
//    #include "cppbridge.h"
// 4. Write functions with EXPOSE() in front
// 5. Run build.bat - DONE!
//
// EXAMPLE:
//   #include "cppbridge.h"
//
//   EXPOSE() const char* greet(const char* name) {
//       return TEXT("Hello, " + std::string(name));
//   }
//
//   EXPOSE() int add(int a, int b) {
//       return a + b;
//   }
//
// That's it! No complex setup. Just EXPOSE() and you're connected to
// JavaScript.
// =============================================================================

#pragma once

#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// =============================================================================
// MAGIC KEYWORDS - Use these in your code
// =============================================================================

// EXPOSE() - Put this before any function to make it callable from JavaScript
#if defined(_WIN32) || defined(_WIN64)
#define EXPOSE() extern "C" __declspec(dllexport)
#elif defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#define EXPOSE() extern "C" EMSCRIPTEN_KEEPALIVE
#else
#define EXPOSE() extern "C" __attribute__((visibility("default")))
#endif

// TEXT() - Use this to return a std::string safely
// Example: return TEXT("Hello " + name);
#if defined(_WIN32) || defined(_WIN64)
#define TEXT(expr)                                                             \
  ([&]() -> const char * {                                                     \
    static std::string _buf;                                                   \
    _buf = (expr);                                                             \
    return _buf.c_str();                                                       \
  })()
#else
#define TEXT(expr)                                                             \
  ([&]() -> const char * {                                                     \
    static thread_local std::string _buf;                                      \
    _buf = (expr);                                                             \
    return _buf.c_str();                                                       \
  })()
#endif

// JSON() - Use this to return JSON data
// Example: return JSON("{\"status\": \"ok\"}");
#define JSON(expr) TEXT(expr)

// =============================================================================
// HELPER FUNCTIONS - Optional utilities
// =============================================================================

namespace CPPBridge {
// Convert any type to string
template <typename T> inline std::string toString(const T &value) {
  std::ostringstream ss;
  ss << value;
  return ss.str();
}

// Build JSON object
inline std::string jsonObject(const std::map<std::string, std::string> &data) {
  std::string result = "{";
  bool first = true;
  for (const auto &pair : data) {
    if (!first)
      result += ",";
    result += "\"" + pair.first + "\":\"" + pair.second + "\"";
    first = false;
  }
  result += "}";
  return result;
}

// Build JSON array
inline std::string jsonArray(const std::vector<std::string> &items) {
  std::string result = "[";
  for (size_t i = 0; i < items.size(); i++) {
    if (i > 0)
      result += ",";
    result += "\"" + items[i] + "\"";
  }
  result += "]";
  return result;
}
} // namespace CPPBridge

// =============================================================================
// BUILT-IN FUNCTIONS (Always available)
// =============================================================================

// Test if bridge is working
EXPOSE() inline const char *bridgeTest() {
  return "✅ CPPBridge is connected!";
}

// Get version
EXPOSE() inline const char *bridgeVersion() { return "2.0.0"; }

// Echo back input (for testing)
EXPOSE() inline const char *bridgeEcho(const char *input) {
  static std::string _buffer;
  _buffer = "Echo: " + std::string(input ? input : "");
  return _buffer.c_str();
}

// =============================================================================
// END OF HEADER - Now just write your functions with EXPOSE()!
// =============================================================================
