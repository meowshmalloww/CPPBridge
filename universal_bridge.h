// =============================================================================
// UNIVERSAL_BRIDGE.H - CPPBridge v3.0 Magic Macro Header
// =============================================================================
//
// THE ONLY HEADER YOU NEED!
//
// Usage:
//   #include "universal_bridge.h"
//
//   BRIDGE_FN(int, add, int a, int b) {
//       return a + b;
//   }
//
//   BRIDGE_FN(const char*, greet, const char* name) {
//       return BRIDGE_STRING("Hello, " + std::string(name));
//   }
//
// Then run: npm run build:bridge
// And use in JS: bridge.add(1, 2)  // returns 3
//
// =============================================================================

#pragma once

#include <cstring>
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
// THE MAGIC MACRO
// =============================================================================
//
// BRIDGE_FN(returnType, functionName, args...)
//
// This macro:
// - Exports the function for DLL (Windows)
// - Registers for JSI (React Native)
// - Exports for WASM (Web)
//
// Examples:
//   BRIDGE_FN(int, add, int a, int b) { return a + b; }
//   BRIDGE_FN(double, multiply, double x, double y) { return x * y; }
//   BRIDGE_FN(const char*, getName) { return "CPPBridge"; }
//
// =============================================================================

#define BRIDGE_FN(returnType, name, ...)                                       \
  BRIDGE_EXPORT returnType name(__VA_ARGS__)

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

// =============================================================================
// CONVENIENCE MACROS
// =============================================================================

// For functions with no parameters
#define BRIDGE_FN_VOID(returnType, name) BRIDGE_EXPORT returnType name()

// For async callbacks (future use)
#define BRIDGE_CALLBACK(name)                                                  \
  typedef void (*name##_callback_t)(const char *result);                       \
  static name##_callback_t _##name##_cb = nullptr;

// =============================================================================
// NOTE: For built-in test functions, users should create their own test
// functions in their src/ files using BRIDGE_FN(). Or use bridge_core.h
// which is the recommended header for v3.1+
// =============================================================================

// =============================================================================
// REACT NATIVE JSI SUPPORT (Conditional)
// =============================================================================

#if defined(BRIDGE_ENABLE_JSI) && __has_include(<jsi/jsi.h>)
#include <jsi/jsi.h>

namespace CPPBridgeJSI {
// Forward declaration - implement this in your React Native setup
void installBindings(facebook::jsi::Runtime &runtime);
} // namespace CPPBridgeJSI

#endif // BRIDGE_ENABLE_JSI

// =============================================================================
// END OF HEADER
// =============================================================================
//
// Now just write your functions:
//
//   BRIDGE_FN(int, calculate, int x, int y, int z) {
//       return x * y + z;
//   }
//
//   BRIDGE_FN(const char*, processText, const char* input) {
//       std::string result = "Processed: ";
//       result += input;
//       return BRIDGE_STRING(result);
//   }
//
// Run: npm run build:bridge
// Use: bridge.calculate(2, 3, 4)  // returns 10
//
// =============================================================================
