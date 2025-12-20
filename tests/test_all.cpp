// =============================================================================
// TEST_ALL.CPP - Comprehensive Test Suite
// =============================================================================

#include "test.h"

// Include modules to test
#include "../hub/core/async.h"
#include "../hub/core/eventbus.h"
#include "../hub/modules/database/keyvalue.h"
#include "../hub/modules/security/crypto.h"
#include "../hub/modules/security/sandbox.h"
#include "../hub/modules/utils/base64.h"


#include <filesystem>

using namespace Hub::Test;

// =============================================================================
// CRYPTO TESTS
// =============================================================================
TEST(Crypto_SHA256_Basic) {
  auto result = Hub::Security::sha256("hello");
  ASSERT_TRUE(result.is_ok());
  ASSERT_EQ(result.data.size(), 32);
}

TEST(Crypto_SHA256_Empty) {
  auto result = Hub::Security::sha256("");
  ASSERT_TRUE(result.is_ok());
  ASSERT_EQ(result.data.size(), 32);
}

TEST(Crypto_AES256_Roundtrip) {
  std::vector<uint8_t> plaintext = {'H', 'e', 'l', 'l', 'o'};
  auto key = Hub::Security::random_bytes(32).data;
  auto iv = Hub::Security::random_bytes(16).data;

  auto encrypted = Hub::Security::aes256_encrypt(plaintext, key, iv);
  ASSERT_TRUE(encrypted.is_ok());
  ASSERT_GT(encrypted.data.size(), 0);

  auto decrypted = Hub::Security::aes256_decrypt(encrypted.data, key, iv);
  ASSERT_TRUE(decrypted.is_ok());
  ASSERT_EQ(decrypted.data.size(), plaintext.size());
  ASSERT_EQ(decrypted.data, plaintext);
}

TEST(Crypto_AES256_LargeData) {
  std::vector<uint8_t> plaintext(1000, 'A');
  auto key = Hub::Security::random_bytes(32).data;
  auto iv = Hub::Security::random_bytes(16).data;

  auto encrypted = Hub::Security::aes256_encrypt(plaintext, key, iv);
  auto decrypted = Hub::Security::aes256_decrypt(encrypted.data, key, iv);
  ASSERT_EQ(decrypted.data, plaintext);
}

TEST(Crypto_Random) {
  auto r1 = Hub::Security::random_bytes(32);
  auto r2 = Hub::Security::random_bytes(32);
  ASSERT_TRUE(r1.is_ok());
  ASSERT_TRUE(r2.is_ok());
  ASSERT_NE(r1.data, r2.data);
}

// =============================================================================
// SANDBOX TESTS
// =============================================================================
TEST(Sandbox_TraversalBlocked) {
  ASSERT_FALSE(Hub::Security::PathValidator::instance().isPathSafe(
      "C:/test/../../../Windows"));
  ASSERT_FALSE(
      Hub::Security::PathValidator::instance().isPathSafe("../secret"));
  ASSERT_FALSE(
      Hub::Security::PathValidator::instance().isPathSafe("..\\secret"));
}

TEST(Sandbox_InputValidation) {
  ASSERT_TRUE(Hub::Security::InputValidator::isValidString("Hello", 100));
  ASSERT_FALSE(Hub::Security::InputValidator::isValidString(nullptr, 100));
  ASSERT_TRUE(Hub::Security::InputValidator::isValidBuffer((void *)1, 100));
  ASSERT_FALSE(Hub::Security::InputValidator::isValidBuffer(nullptr, 100));
}

// =============================================================================
// BASE64 TESTS
// =============================================================================
TEST(Base64_EncodeBasic) {
  std::string encoded = Hub::Utils::Base64::encode("Hello");
  ASSERT_STREQ(encoded.c_str(), "SGVsbG8=");
}

TEST(Base64_Roundtrip) {
  std::string original = "Test data for base64 encoding!";
  std::string encoded = Hub::Utils::Base64::encode(original);
  std::string decoded = Hub::Utils::Base64::decodeToString(encoded);
  ASSERT_STREQ(decoded.c_str(), original.c_str());
}

// =============================================================================
// ASYNC TESTS
// =============================================================================
TEST(Async_Initialize) {
  Hub::AsyncRunner::instance().init(4);
  ASSERT_TRUE(Hub::AsyncRunner::instance().is_initialized());
  ASSERT_EQ(Hub::AsyncRunner::instance().worker_count(), 4);
}

TEST(Async_SubmitBlocking) {
  int result =
      Hub::AsyncRunner::instance().submit_blocking([]() { return 42; });
  ASSERT_EQ(result, 42);
}

// =============================================================================
// EVENT BUS TESTS
// =============================================================================
TEST(EventBus_PublishPoll) {
  Hub::EventBus::instance().clear();
  Hub::EventBus::instance().publish(Hub::EventType::INIT_COMPLETE, "test",
                                    "data");

  ASSERT_EQ(Hub::EventBus::instance().pendingCount(), 1);

  Hub::Event event;
  bool got = Hub::EventBus::instance().poll(event);
  ASSERT_TRUE(got);
  ASSERT_EQ(event.type, Hub::EventType::INIT_COMPLETE);
  ASSERT_STREQ(event.name.c_str(), "test");
}

// =============================================================================
// KEYVALUE TESTS
// =============================================================================
TEST(KeyValue_SetGet) {
  Hub::Database::KeyValueStore store;
  store.set("key1", "value1");

  auto val = store.get("key1");
  ASSERT_TRUE(val.has_value());
  ASSERT_STREQ(val->c_str(), "value1");
}

TEST(KeyValue_NotFound) {
  Hub::Database::KeyValueStore store;
  auto val = store.get("nonexistent");
  ASSERT_FALSE(val.has_value());
}

// =============================================================================
// MAIN
// =============================================================================
int main() {
  std::cout << "UniversalBridge Unit Tests\n";
  std::cout << "==========================\n";

  int failures = RUN_ALL_TESTS();

  // Cleanup
  Hub::AsyncRunner::instance().shutdown(false);

  return failures;
}
