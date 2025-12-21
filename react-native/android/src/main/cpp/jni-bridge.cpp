// =============================================================================
// JNI Bridge - Connects Java to CPPBridge
// =============================================================================
// This file is only compiled on Android with the NDK.
// The IDE may show errors if jni.h is not found - this is expected.
// =============================================================================

#ifdef __ANDROID__

#include <cstring>
#include <jni.h>
#include <string>


// Include CPPBridge headers
#include "modules/database/database.h"
#include "modules/database/keyvalue.h"
#include "modules/security/crypto.h"


extern "C" {

// =============================================================================
// HTTP Methods (placeholder - use Java HttpURLConnection for Android)
// =============================================================================
JNIEXPORT jstring JNICALL Java_com_cppbridge_CppBridgeModule_nativeHttpGet(
    JNIEnv *env, jobject, jstring url) {
  const char *urlStr = env->GetStringUTFChars(url, nullptr);
  // Note: For Android, prefer using Java's HttpURLConnection
  std::string result =
      "{\"status\": 0, \"body\": \"Use Java HTTP client for Android\"}";
  env->ReleaseStringUTFChars(url, urlStr);
  return env->NewStringUTF(result.c_str());
}

JNIEXPORT jstring JNICALL Java_com_cppbridge_CppBridgeModule_nativeHttpPost(
    JNIEnv *env, jobject, jstring url, jstring body, jstring contentType) {
  const char *urlStr = env->GetStringUTFChars(url, nullptr);
  const char *bodyStr = env->GetStringUTFChars(body, nullptr);
  const char *ctStr = env->GetStringUTFChars(contentType, nullptr);

  std::string result =
      "{\"status\": 0, \"body\": \"Use Java HTTP client for Android\"}";

  env->ReleaseStringUTFChars(url, urlStr);
  env->ReleaseStringUTFChars(body, bodyStr);
  env->ReleaseStringUTFChars(contentType, ctStr);
  return env->NewStringUTF(result.c_str());
}

// =============================================================================
// Database Methods
// =============================================================================
JNIEXPORT jint JNICALL Java_com_cppbridge_CppBridgeModule_nativeDbOpen(
    JNIEnv *env, jobject, jstring path) {
  const char *pathStr = env->GetStringUTFChars(path, nullptr);
  int handle = hub_db_open(pathStr);
  env->ReleaseStringUTFChars(path, pathStr);
  return handle;
}

JNIEXPORT jboolean JNICALL Java_com_cppbridge_CppBridgeModule_nativeDbExec(
    JNIEnv *env, jobject, jint handle, jstring sql) {
  const char *sqlStr = env->GetStringUTFChars(sql, nullptr);
  int result = hub_db_exec(handle, sqlStr);
  env->ReleaseStringUTFChars(sql, sqlStr);
  return result == 0;
}

JNIEXPORT jstring JNICALL Java_com_cppbridge_CppBridgeModule_nativeDbQuery(
    JNIEnv *env, jobject, jint handle, jstring sql) {
  const char *sqlStr = env->GetStringUTFChars(sql, nullptr);
  const char *result = hub_db_query(handle, sqlStr);
  env->ReleaseStringUTFChars(sql, sqlStr);
  return env->NewStringUTF(result ? result : "[]");
}

JNIEXPORT void JNICALL Java_com_cppbridge_CppBridgeModule_nativeDbClose(
    JNIEnv *, jobject, jint handle) {
  hub_db_close(handle);
}

// =============================================================================
// Key-Value Store Methods
// =============================================================================
JNIEXPORT jint JNICALL Java_com_cppbridge_CppBridgeModule_nativeKvOpen(
    JNIEnv *env, jobject, jstring path) {
  const char *pathStr = env->GetStringUTFChars(path, nullptr);
  int handle = hub_kv_open(pathStr);
  env->ReleaseStringUTFChars(path, pathStr);
  return handle;
}

JNIEXPORT jboolean JNICALL Java_com_cppbridge_CppBridgeModule_nativeKvSet(
    JNIEnv *env, jobject, jint handle, jstring key, jstring value) {
  const char *keyStr = env->GetStringUTFChars(key, nullptr);
  const char *valueStr = env->GetStringUTFChars(value, nullptr);
  int result = hub_kv_set(handle, keyStr, valueStr);
  env->ReleaseStringUTFChars(key, keyStr);
  env->ReleaseStringUTFChars(value, valueStr);
  return result == 1;
}

JNIEXPORT jstring JNICALL Java_com_cppbridge_CppBridgeModule_nativeKvGet(
    JNIEnv *env, jobject, jint handle, jstring key) {
  const char *keyStr = env->GetStringUTFChars(key, nullptr);
  const char *result = hub_kv_get(handle, keyStr);
  env->ReleaseStringUTFChars(key, keyStr);
  return result ? env->NewStringUTF(result) : nullptr;
}

JNIEXPORT jboolean JNICALL Java_com_cppbridge_CppBridgeModule_nativeKvDelete(
    JNIEnv *env, jobject, jint handle, jstring key) {
  const char *keyStr = env->GetStringUTFChars(key, nullptr);
  int result = hub_kv_delete(handle, keyStr);
  env->ReleaseStringUTFChars(key, keyStr);
  return result == 1;
}

JNIEXPORT void JNICALL Java_com_cppbridge_CppBridgeModule_nativeKvClose(
    JNIEnv *, jobject, jint handle) {
  hub_kv_close(handle);
}

// =============================================================================
// Crypto Methods
// =============================================================================
JNIEXPORT jstring JNICALL Java_com_cppbridge_CppBridgeModule_nativeCryptoSha256(
    JNIEnv *env, jobject, jstring data) {
  const char *dataStr = env->GetStringUTFChars(data, nullptr);

  // Call CPPBridge crypto
  unsigned char hash[32];
  int len = static_cast<int>(strlen(dataStr));
  hub_crypto_sha256(dataStr, len, hash, 32);

  // Convert to hex string
  char hexStr[65];
  for (int i = 0; i < 32; i++) {
    sprintf(hexStr + (i * 2), "%02x", hash[i]);
  }
  hexStr[64] = '\0';

  env->ReleaseStringUTFChars(data, dataStr);
  return env->NewStringUTF(hexStr);
}

} // extern "C"

#endif // __ANDROID__
