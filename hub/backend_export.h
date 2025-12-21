// =============================================================================
// BACKEND_EXPORT.H - Universal C++ Export Header
// =============================================================================
// Paste this at the top of ANY C++ file to expose functions to JavaScript.
//
// Usage:
//   #include "backend_export.h"
//
//   BACKEND_API const char* greet(const char* name) {
//       static std::string result;
//       result = "Hello, " + std::string(name) + "!";
//       return result.c_str();
//   }
//
// Works with: Electron (koffi/ffi-napi), React Native, WebAssembly, Node.js
// =============================================================================

#pragma once

#include <cstring>
#include <string>


// =============================================================================
// PLATFORM DETECTION & EXPORT MACRO
// =============================================================================

#if defined(_WIN32) || defined(_WIN64)
// Windows: Use __declspec(dllexport)
#define BACKEND_API extern "C" __declspec(dllexport)
#define BACKEND_PLATFORM_WINDOWS
#elif defined(__EMSCRIPTEN__)
// WebAssembly: Use EMSCRIPTEN_KEEPALIVE
#include <emscripten/emscripten.h>
#define BACKEND_API extern "C" EMSCRIPTEN_KEEPALIVE
#define BACKEND_PLATFORM_WASM
#elif defined(__ANDROID__)
// Android: Functions exposed via JNI
#define BACKEND_API extern "C" __attribute__((visibility("default")))
#define BACKEND_PLATFORM_ANDROID
#elif defined(__APPLE__)
// macOS/iOS: Standard visibility
#define BACKEND_API extern "C" __attribute__((visibility("default")))
#define BACKEND_PLATFORM_APPLE
#else
// Linux and others
#define BACKEND_API extern "C" __attribute__((visibility("default")))
#define BACKEND_PLATFORM_UNIX
#endif

// =============================================================================
// STRING HELPER - Return std::string safely to JavaScript
// =============================================================================
// JavaScript expects const char*, but std::string goes out of scope.
// This helper stores the result statically so it survives the function return.
//
// Usage:
//   BACKEND_API const char* myFunction() {
//       std::string result = "Hello World";
//       return RETURN_STRING(result);
//   }
// =============================================================================

// Thread-local storage for returned strings (thread-safe)
#ifdef BACKEND_PLATFORM_WINDOWS
#define RETURN_STRING(str)                                                     \
  static thread_local std::string _return_buffer;                              \
  _return_buffer = (str);                                                      \
  return _return_buffer.c_str()
#else
#define RETURN_STRING(str)                                                     \
  static __thread std::string _return_buffer;                                  \
  _return_buffer = (str);                                                      \
  return _return_buffer.c_str()
#endif

// Alternative: Caller-allocated buffer (for large strings)
// Usage: BACKEND_API int myFunction(char* buffer, int bufferSize) { ... }
inline int COPY_TO_BUFFER(const std::string &str, char *buffer,
                          int bufferSize) {
  if (!buffer || bufferSize <= 0)
    return -1;
  int copyLen = static_cast<int>(str.length());
  if (copyLen >= bufferSize)
    copyLen = bufferSize - 1;
  std::memcpy(buffer, str.c_str(), copyLen);
  buffer[copyLen] = '\0';
  return copyLen;
}

// =============================================================================
// CONDITIONAL INCLUDES - Only include when platform is available
// =============================================================================

// React Native JSI (only on mobile)
#if !defined(BACKEND_PLATFORM_WINDOWS) && !defined(BACKEND_PLATFORM_WASM)
#if __has_include(<jsi/jsi.h>)
#define BACKEND_HAS_JSI 1
// #include <jsi/jsi.h>  // Uncomment when using React Native
#endif
#endif

// =============================================================================
// EXAMPLE FUNCTIONS (Delete these in your own file)
// =============================================================================

#ifdef BACKEND_INCLUDE_EXAMPLES

// Simple function returning an integer
BACKEND_API int add(int a, int b) { return a + b; }

// Function returning a string
BACKEND_API const char *greet(const char *name) {
  static std::string result;
  result = "Hello, " + std::string(name) + "!";
  return result.c_str();
}

// Test function for button clicks
BACKEND_API const char *backendMessage() {
  RETURN_STRING("Backend is connected!");
}

// Version info
BACKEND_API const char *getVersion() { RETURN_STRING("1.0.0"); }

#endif // BACKEND_INCLUDE_EXAMPLES
