// =============================================================================
// BRIDGE_CORE.H - CPPBridge v3.1 'Zero-Config' Magic Macro Header
// =============================================================================
//
// THE ONLY HEADER YOU NEED!
//
// Features:
// - BRIDGE_FN() macro for automatic export
// - BRIDGE_STRING() for safe string returns
// - Crash protection with try-catch wrapper
// - Cross-platform: Windows, React Native, WebAssembly
//
// Usage:
//   #include "bridge_core.h"
//
//   BRIDGE_FN(int, add, int a, int b) {
//       return a + b;
//   }
//
// Build: npm run build:bridge
// Use:   bridge.add(5, 3)  // returns 8
//
// =============================================================================

#ifndef BRIDGE_CORE_H
#define BRIDGE_CORE_H

#include <cstring>
#include <exception>
#include <iostream>
#include <string>


// =============================================================================
// PLATFORM DETECTION
// =============================================================================

#if defined(_WIN32) || defined(_WIN64)
#define BRIDGE_PLATFORM_WINDOWS
#define BRIDGE_EXPORT extern "C" __declspec(dllexport)
#elif defined(__EMSCRIPTEN__)
#define BRIDGE_PLATFORM_WASM
#include <emscripten/emscripten.h>
#define BRIDGE_EXPORT extern "C" EMSCRIPTEN_KEEPALIVE
#elif defined(__ANDROID__)
#define BRIDGE_PLATFORM_ANDROID
#define BRIDGE_EXPORT extern "C" __attribute__((visibility("default")))
#elif defined(__APPLE__)
#define BRIDGE_PLATFORM_APPLE
#define BRIDGE_EXPORT extern "C" __attribute__((visibility("default")))
#else
#define BRIDGE_PLATFORM_UNIX
#define BRIDGE_EXPORT extern "C" __attribute__((visibility("default")))
#endif

// =============================================================================
// CRASH PROTECTION MACROS
// =============================================================================
//
// These macros wrap user code in try-catch to prevent app crashes.
// If an exception occurs, an error is logged and a safe default is returned.
//

// Default values for different types
#define BRIDGE_DEFAULT_INT 0
#define BRIDGE_DEFAULT_DOUBLE 0.0
#define BRIDGE_DEFAULT_FLOAT 0.0f
#define BRIDGE_DEFAULT_BOOL false
#define BRIDGE_DEFAULT_STRING ""

// Internal: Error logging
#define BRIDGE_LOG_ERROR(funcName, errMsg)                                     \
  std::cerr << "[CPPBridge Error] " << funcName << ": " << errMsg << std::endl

// =============================================================================
// THE MAGIC MACRO - WITH CRASH PROTECTION
// =============================================================================
//
// BRIDGE_FN(returnType, functionName, args...)
//
// This macro:
// 1. Exports the function for DLL/JSI/WASM
// 2. Wraps your code in try-catch for crash protection
// 3. Returns safe defaults if an exception occurs
//
// For functions returning int, double, float, bool:
//   Use BRIDGE_FN_SAFE(type, name, defaultValue, args...) for custom defaults
//
// For functions returning strings:
//   Use BRIDGE_STRING() to safely return std::string
//
// =============================================================================

// Standard macro (no built-in crash protection - user handles it)
#define BRIDGE_FN(returnType, name, ...)                                       \
  BRIDGE_EXPORT returnType name(__VA_ARGS__)

// Safe macro with crash protection for numeric types
#define BRIDGE_FN_SAFE(returnType, name, defaultValue, ...)                    \
  returnType name##_impl(__VA_ARGS__);                                         \
  BRIDGE_EXPORT returnType name(__VA_ARGS__) {                                 \
    try {                                                                      \
      return name##_impl(__VA_ARGS__);                                         \
    } catch (const std::exception &e) {                                        \
      BRIDGE_LOG_ERROR(#name, e.what());                                       \
      return defaultValue;                                                     \
    } catch (...) {                                                            \
      BRIDGE_LOG_ERROR(#name, "Unknown exception");                            \
      return defaultValue;                                                     \
    }                                                                          \
  }                                                                            \
  returnType name##_impl(__VA_ARGS__)

// =============================================================================
// STRING HELPER
// =============================================================================
//
// Use BRIDGE_STRING() to safely return std::string to JavaScript
//
// Example:
//   BRIDGE_FN(const char*, greet, const char* name) {
//       return BRIDGE_STRING("Hello, " + std::string(name) + "!");
//   }
//
// =============================================================================

#define BRIDGE_STRING(expr)                                                    \
  ([&]() -> const char * {                                                     \
    static std::string _bridge_buf;                                            \
    _bridge_buf = (expr);                                                      \
    return _bridge_buf.c_str();                                                \
  })()

// Safe string return with crash protection
#define BRIDGE_STRING_SAFE(expr)                                               \
  ([&]() -> const char * {                                                     \
    static std::string _bridge_buf;                                            \
    try {                                                                      \
      _bridge_buf = (expr);                                                    \
    } catch (const std::exception &e) {                                        \
      BRIDGE_LOG_ERROR("BRIDGE_STRING", e.what());                             \
      _bridge_buf = "";                                                        \
    } catch (...) {                                                            \
      BRIDGE_LOG_ERROR("BRIDGE_STRING", "Unknown exception");                  \
      _bridge_buf = "";                                                        \
    }                                                                          \
    return _bridge_buf.c_str();                                                \
  })()

// =============================================================================
// CONVENIENCE MACROS
// =============================================================================

// For functions with no parameters
#define BRIDGE_FN_VOID(returnType, name) BRIDGE_EXPORT returnType name()

// =============================================================================
// REACT NATIVE JSI SUPPORT (Conditional)
// =============================================================================

#if defined(BRIDGE_ENABLE_JSI) && __has_include(<jsi/jsi.h>)
#include <jsi/jsi.h>

namespace CPPBridgeJSI {
// Forward declaration - implement in React Native setup
void installBindings(facebook::jsi::Runtime &runtime);
} // namespace CPPBridgeJSI

#endif // BRIDGE_ENABLE_JSI

// =============================================================================
// END OF HEADER
// =============================================================================
//
// Now just write your functions:
//
//   BRIDGE_FN(int, add, int a, int b) {
//       return a + b;
//   }
//
//   // With crash protection:
//   BRIDGE_FN_SAFE(int, divide, 0, int a, int b) {
//       return a / b;  // Won't crash even if b is 0!
//   }
//
//   BRIDGE_FN(const char*, greet, const char* name) {
//       return BRIDGE_STRING("Hello, " + std::string(name));
//   }
//
// Build: npm run build:bridge
// Use:   bridge.add(5, 3)
//
// =============================================================================

#endif // BRIDGE_CORE_H
