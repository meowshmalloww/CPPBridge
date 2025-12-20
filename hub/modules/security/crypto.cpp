// =============================================================================
// CRYPTO.CPP - Cryptography Implementation
// =============================================================================

#include "crypto.h"
#include <algorithm>
#include <cstring>


static thread_local std::string tl_hash_result;

extern "C" {

__declspec(dllexport) int hub_crypto_sha256(const char *input, char *output,
                                            int output_size) {
  if (!input)
    return -1;

  auto result = Hub::Security::sha256(std::string(input));
  if (!result.is_ok())
    return -1;

  tl_hash_result = result.toHex();

  if (output && output_size > 0) {
    size_t copy_size =
        (std::min)(static_cast<size_t>(output_size - 1), tl_hash_result.size());
    std::memcpy(output, tl_hash_result.data(), copy_size);
    output[copy_size] = '\0';
  }

  return static_cast<int>(tl_hash_result.size());
}

__declspec(dllexport) int hub_crypto_random(int count, unsigned char *output) {
  if (!output || count <= 0)
    return -1;

  auto result = Hub::Security::random_bytes(static_cast<size_t>(count));
  if (!result.is_ok())
    return -1;

  std::memcpy(output, result.data.data(), count);
  return count;
}

__declspec(dllexport) int
hub_crypto_aes_encrypt(const unsigned char *data, int data_len,
                       const unsigned char *key, const unsigned char *iv,
                       unsigned char *output, int output_size) {
  if (!data || !key || !iv || !output || data_len <= 0)
    return -1;

  std::vector<uint8_t> plaintext(data, data + data_len);
  std::vector<uint8_t> keyVec(key, key + 32);
  std::vector<uint8_t> ivVec(iv, iv + 16);

  auto result = Hub::Security::aes256_encrypt(plaintext, keyVec, ivVec);
  if (!result.is_ok())
    return -1;

  int needed = static_cast<int>(result.data.size());
  if (output_size < needed)
    return needed; // Return required size

  std::memcpy(output, result.data.data(), result.data.size());
  return needed;
}

__declspec(dllexport) int
hub_crypto_aes_decrypt(const unsigned char *data, int data_len,
                       const unsigned char *key, const unsigned char *iv,
                       unsigned char *output, int output_size) {
  if (!data || !key || !iv || !output || data_len <= 0)
    return -1;

  std::vector<uint8_t> ciphertext(data, data + data_len);
  std::vector<uint8_t> keyVec(key, key + 32);
  std::vector<uint8_t> ivVec(iv, iv + 16);

  auto result = Hub::Security::aes256_decrypt(ciphertext, keyVec, ivVec);
  if (!result.is_ok())
    return -1;

  int needed = static_cast<int>(result.data.size());
  if (output_size < needed)
    return needed;

  std::memcpy(output, result.data.data(), result.data.size());
  return needed;
}

__declspec(dllexport) int
hub_crypto_pbkdf2(const char *password, const unsigned char *salt, int salt_len,
                  int iterations, unsigned char *output, int output_len) {
  if (!password || !salt || !output || salt_len <= 0 || output_len <= 0 ||
      iterations <= 0)
    return -1;

  std::vector<uint8_t> saltVec(salt, salt + salt_len);
  auto result =
      Hub::Security::pbkdf2_sha256(password, saltVec, iterations, output_len);

  std::memcpy(output, result.data(), result.size());
  return output_len;
}

} // extern "C"
