#pragma once
// =============================================================================
// TYPES.H - Core Type Definitions for UniversalBridge
// =============================================================================
// Provides unified error handling and result types across all modules.
// =============================================================================

#include <string>

namespace Hub {

// =============================================================================
// ERROR CODES
// =============================================================================
enum class ErrorCode {
  OK = 0,

  // General (1-99)
  UNKNOWN = 1,
  INVALID_HANDLE = 2,
  NULL_POINTER = 3,
  INVALID_ARGUMENT = 4,
  NOT_IMPLEMENTED = 5,
  TIMEOUT = 6,
  CANCELLED = 7,

  // File System (100-199)
  FILE_NOT_FOUND = 100,
  FILE_ACCESS_DENIED = 101,
  FILE_ALREADY_EXISTS = 102,
  FILE_IN_USE = 103,
  DIRECTORY_NOT_FOUND = 104,
  DISK_FULL = 105,
  PATH_TOO_LONG = 106,

  // Network (200-299)
  NETWORK_ERROR = 200,
  CONNECTION_FAILED = 201,
  CONNECTION_TIMEOUT = 202,
  DNS_RESOLUTION_FAILED = 203,
  SSL_ERROR = 204,
  HTTP_ERROR = 205,
  WEBSOCKET_ERROR = 206,

  // Database (300-399)
  DB_ERROR = 300,
  DB_CONNECTION_FAILED = 301,
  DB_QUERY_FAILED = 302,
  DB_CONSTRAINT_VIOLATION = 303,
  DB_NOT_FOUND = 304,

  // Security (400-499)
  CRYPTO_ERROR = 400,
  ENCRYPTION_FAILED = 401,
  DECRYPTION_FAILED = 402,
  HASH_FAILED = 403,
  INVALID_KEY = 404,

  // System (500-599)
  SYSTEM_ERROR = 500,
  PERMISSION_DENIED = 501,
  RESOURCE_UNAVAILABLE = 502,
  PROCESS_NOT_FOUND = 503
};

// =============================================================================
// ERROR STRUCTURE
// =============================================================================
struct Error {
  ErrorCode code = ErrorCode::OK;
  std::string message;
  std::string context; // e.g., "network::http_get", "filesystem::backup"

  Error() = default;
  Error(ErrorCode c, const std::string &msg, const std::string &ctx = "")
      : code(c), message(msg), context(ctx) {}

  bool is_ok() const { return code == ErrorCode::OK; }

  // Convert to JSON string for FFI
  std::string to_json() const {
    if (is_ok())
      return "{}";
    return "{\"error\":{\"code\":" + std::to_string(static_cast<int>(code)) +
           ",\"message\":\"" + escape_json(message) + "\"" +
           (context.empty() ? ""
                            : ",\"context\":\"" + escape_json(context) + "\"") +
           "}}";
  }

private:
  static std::string escape_json(const std::string &s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
      switch (c) {
      case '"':
        result += "\\\"";
        break;
      case '\\':
        result += "\\\\";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        result += c;
        break;
      }
    }
    return result;
  }
};

// =============================================================================
// RESULT TYPE - Success or Error
// =============================================================================
template <typename T> struct Result {
  bool success;
  T value;
  Error error;

  // Success constructor
  static Result Ok(const T &val) {
    Result r;
    r.success = true;
    r.value = val;
    return r;
  }

  static Result Ok(T &&val) {
    Result r;
    r.success = true;
    r.value = std::move(val);
    return r;
  }

  // Error constructor
  static Result Err(ErrorCode code, const std::string &msg,
                    const std::string &ctx = "") {
    Result r;
    r.success = false;
    r.error = Error(code, msg, ctx);
    return r;
  }

  static Result Err(const Error &err) {
    Result r;
    r.success = false;
    r.error = err;
    return r;
  }

  // Accessors
  bool is_ok() const { return success; }
  bool is_err() const { return !success; }

  const T &unwrap() const { return value; }
  T &unwrap() { return value; }

  const T &unwrap_or(const T &default_val) const {
    return success ? value : default_val;
  }
};

// Specialization for void results
template <> struct Result<void> {
  bool success;
  Error error;

  static Result Ok() {
    Result r;
    r.success = true;
    return r;
  }

  static Result Err(ErrorCode code, const std::string &msg,
                    const std::string &ctx = "") {
    Result r;
    r.success = false;
    r.error = Error(code, msg, ctx);
    return r;
  }

  bool is_ok() const { return success; }
  bool is_err() const { return !success; }
};

// =============================================================================
// CALLBACK TYPES
// =============================================================================
#ifndef HUB_TASK_ID_DEFINED
using TaskId = uint32_t;
using ResultCallback = void (*)(TaskId task_id, int result_handle,
                                const char *error_json);
#define HUB_TASK_ID_DEFINED
#endif

} // namespace Hub
