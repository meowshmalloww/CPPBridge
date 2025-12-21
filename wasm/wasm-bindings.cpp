// =============================================================================
// WASM Bindings - Exposes CPPBridge to JavaScript via WebAssembly
// =============================================================================
// This file is only compiled with Emscripten (emcc/em++).
// The IDE may show errors if Emscripten isn't installed - this is expected.
// Build with: emcmake cmake .. && emmake make
// =============================================================================

#ifdef __EMSCRIPTEN__

#include <cstring>
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <string>


// Include CPPBridge headers
#include "modules/database/database.h"
#include "modules/database/keyvalue.h"
#include "modules/security/crypto.h"
#include "modules/utils/base64.h"
#include "modules/utils/compress.h"


using namespace emscripten;

// =============================================================================
// Database Wrapper
// =============================================================================
class DatabaseWrapper {
public:
  int open(const std::string &path) { return hub_db_open(path.c_str()); }

  bool exec(int handle, const std::string &sql) {
    return hub_db_exec(handle, sql.c_str()) == 0;
  }

  std::string query(int handle, const std::string &sql) {
    const char *result = hub_db_query(handle, sql.c_str());
    return result ? std::string(result) : "[]";
  }

  void close(int handle) { hub_db_close(handle); }
};

// =============================================================================
// KeyValue Wrapper
// =============================================================================
class KeyValueWrapper {
public:
  int open(const std::string &path) { return hub_kv_open(path.c_str()); }

  bool set(int handle, const std::string &key, const std::string &value) {
    return hub_kv_set(handle, key.c_str(), value.c_str()) == 1;
  }

  std::string get(int handle, const std::string &key) {
    const char *result = hub_kv_get(handle, key.c_str());
    return result ? std::string(result) : "";
  }

  bool remove(int handle, const std::string &key) {
    return hub_kv_delete(handle, key.c_str()) == 1;
  }

  void close(int handle) { hub_kv_close(handle); }
};

// =============================================================================
// Crypto Wrapper
// =============================================================================
class CryptoWrapper {
public:
  std::string sha256(const std::string &data) {
    unsigned char hash[32];
    hub_crypto_sha256(data.c_str(), static_cast<int>(data.length()), hash, 32);

    char hexStr[65];
    for (int i = 0; i < 32; i++) {
      sprintf(hexStr + (i * 2), "%02x", hash[i]);
    }
    hexStr[64] = '\0';
    return std::string(hexStr);
  }

  std::string base64Encode(const std::string &data) {
    const char *result =
        hub_base64_encode(data.c_str(), static_cast<int>(data.length()));
    return result ? std::string(result) : "";
  }

  std::string base64Decode(const std::string &data) {
    int outLen = 0;
    const char *result = hub_base64_decode(
        data.c_str(), static_cast<int>(data.length()), &outLen);
    return result ? std::string(result, outLen) : "";
  }
};

// =============================================================================
// Emscripten Bindings
// =============================================================================
EMSCRIPTEN_BINDINGS(cppbridge) {
  class_<DatabaseWrapper>("Database")
      .constructor<>()
      .function("open", &DatabaseWrapper::open)
      .function("exec", &DatabaseWrapper::exec)
      .function("query", &DatabaseWrapper::query)
      .function("close", &DatabaseWrapper::close);

  class_<KeyValueWrapper>("KeyValue")
      .constructor<>()
      .function("open", &KeyValueWrapper::open)
      .function("set", &KeyValueWrapper::set)
      .function("get", &KeyValueWrapper::get)
      .function("remove", &KeyValueWrapper::remove)
      .function("close", &KeyValueWrapper::close);

  class_<CryptoWrapper>("Crypto")
      .constructor<>()
      .function("sha256", &CryptoWrapper::sha256)
      .function("base64Encode", &CryptoWrapper::base64Encode)
      .function("base64Decode", &CryptoWrapper::base64Decode);
}

#endif // __EMSCRIPTEN__
