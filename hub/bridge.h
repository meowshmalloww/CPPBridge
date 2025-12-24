#pragma once
// =============================================================================
// BRIDGE.H - Comprehensive C++ to JS Bridge System v2.0
// =============================================================================
//
// Features:
//   - Functions    : BRIDGE int add(int a, int b)
//   - Variables    : BRIDGE_VAR(int, counter, 0)
//   - Enums        : BRIDGE_ENUM(Color, Red, Green, Blue)
//   - Structs      : BRIDGE_STRUCT(Point, int x, int y)
//   - Classes      : Standard C++ classes with BRIDGE methods
//   - Exceptions   : BRIDGE_SAFE + BRIDGE_THROW
//   - Callbacks    : BRIDGE_CALLBACK
//
// =============================================================================

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// =============================================================================
// PLATFORM DETECTION
// =============================================================================

#ifdef _WIN32
#define BRIDGE_EXPORT extern "C" __declspec(dllexport)
#else
#define BRIDGE_EXPORT extern "C" __attribute__((visibility("default")))
#endif

// =============================================================================
// BASIC TYPES
// =============================================================================

typedef const char *str;

// Thread-local string storage for safe returns
static thread_local std::string _bridge_str;
#define RET(x) (_bridge_str = (x), _bridge_str.c_str())

// =============================================================================
// ZERO-COPY BUFFERS - Direct memory access for AI/Video
// =============================================================================
// Usage: BRIDGE void processImage(BRIDGE_BUFFER(data, size)) { ... }
// JS:    const img = new Uint8Array(1024); bridge.processImage(img,
// img.length);

#define BRIDGE_BUFFER(name, lenName) uint8_t *name, int lenName

// =============================================================================
// 1. FUNCTIONS - Basic function export
// =============================================================================
// Usage: BRIDGE int add(int a, int b) { return a + b; }

#define BRIDGE BRIDGE_EXPORT

// =============================================================================
// 2. VARIABLES - Auto getter/setter generation
// =============================================================================
// Usage: BRIDGE_VAR(int, counter, 0)
// JS:    bridge.get_counter() / bridge.set_counter(5)

#define BRIDGE_VAR(type, name, initial)                                        \
  static type _var_##name = initial;                                           \
  BRIDGE_EXPORT type get_##name() { return _var_##name; }                      \
  BRIDGE_EXPORT void set_##name(type value) { _var_##name = value; }

#define BRIDGE_VAR_STR(name, initial)                                          \
  static std::string _var_##name = initial;                                    \
  BRIDGE_EXPORT str get_##name() { return _var_##name.c_str(); }               \
  BRIDGE_EXPORT void set_##name(str value) { _var_##name = value ? value : ""; }

// =============================================================================
// 3. ENUMS - Export enum values
// =============================================================================

#define BRIDGE_ENUM_VALUE(enumName, valueName, index)                          \
  BRIDGE_EXPORT int enumName##_##valueName() { return index; }

#define BRIDGE_ENUM_1(name, v0)                                                \
  enum name { v0 };                                                            \
  BRIDGE_ENUM_VALUE(name, v0, 0)

#define BRIDGE_ENUM_2(name, v0, v1)                                            \
  enum name { v0, v1 };                                                        \
  BRIDGE_ENUM_VALUE(name, v0, 0)                                               \
  BRIDGE_ENUM_VALUE(name, v1, 1)

#define BRIDGE_ENUM_3(name, v0, v1, v2)                                        \
  enum name { v0, v1, v2 };                                                    \
  BRIDGE_ENUM_VALUE(name, v0, 0)                                               \
  BRIDGE_ENUM_VALUE(name, v1, 1)                                               \
  BRIDGE_ENUM_VALUE(name, v2, 2)

#define BRIDGE_ENUM_4(name, v0, v1, v2, v3)                                    \
  enum name { v0, v1, v2, v3 };                                                \
  BRIDGE_ENUM_VALUE(name, v0, 0)                                               \
  BRIDGE_ENUM_VALUE(name, v1, 1)                                               \
  BRIDGE_ENUM_VALUE(name, v2, 2)                                               \
  BRIDGE_ENUM_VALUE(name, v3, 3)

#define BRIDGE_ENUM_5(name, v0, v1, v2, v3, v4)                                \
  enum name { v0, v1, v2, v3, v4 };                                            \
  BRIDGE_ENUM_VALUE(name, v0, 0)                                               \
  BRIDGE_ENUM_VALUE(name, v1, 1)                                               \
  BRIDGE_ENUM_VALUE(name, v2, 2)                                               \
  BRIDGE_ENUM_VALUE(name, v3, 3)                                               \
  BRIDGE_ENUM_VALUE(name, v4, 4)

// =============================================================================
// 4. STRUCTS - Serializable to JSON
// =============================================================================

#define BRIDGE_STRUCT_2(name, t1, n1, t2, n2)                                  \
  struct name {                                                                \
    t1 n1;                                                                     \
    t2 n2;                                                                     \
  };                                                                           \
  BRIDGE_EXPORT str name##_create(t1 n1, t2 n2) {                              \
    std::ostringstream ss;                                                     \
    ss << "{\"" #n1 "\":" << n1 << ",\"" #n2 "\":" << n2 << "}";               \
    return RET(ss.str());                                                      \
  }

#define BRIDGE_STRUCT_3(name, t1, n1, t2, n2, t3, n3)                          \
  struct name {                                                                \
    t1 n1;                                                                     \
    t2 n2;                                                                     \
    t3 n3;                                                                     \
  };                                                                           \
  BRIDGE_EXPORT str name##_create(t1 n1, t2 n2, t3 n3) {                       \
    std::ostringstream ss;                                                     \
    ss << "{\"" #n1 "\":" << n1 << ",\"" #n2 "\":" << n2                       \
       << ",\"" #n3 "\":" << n3 << "}";                                        \
    return RET(ss.str());                                                      \
  }

// =============================================================================
// 5. EXCEPTIONS - Safe function wrapper
// =============================================================================

extern std::string _bridge_error;
extern bool _bridge_has_error;

#define BRIDGE_THROW(msg)                                                      \
  do {                                                                         \
    _bridge_error = msg;                                                       \
    _bridge_has_error = true;                                                  \
    return {};                                                                 \
  } while (0)

#define BRIDGE_SAFE BRIDGE_EXPORT

// =============================================================================
// 6. CALLBACKS - Stored, polled from JS
// =============================================================================

extern std::vector<std::string> _bridge_callback_queue;

#define BRIDGE_CALLBACK(name, paramType)                                       \
  inline void call_##name(paramType value) {                                   \
    std::ostringstream ss;                                                     \
    ss << "{\"callback\":\"" #name "\",\"value\":" << value << "}";            \
    _bridge_callback_queue.push_back(ss.str());                                \
  }

#define BRIDGE_CALLBACK_STR(name)                                              \
  inline void call_##name(str value) {                                         \
    std::ostringstream ss;                                                     \
    ss << "{\"callback\":\"" #name "\",\"value\":\"" << (value ? value : "")   \
       << "\"}";                                                               \
    _bridge_callback_queue.push_back(ss.str());                                \
  }
