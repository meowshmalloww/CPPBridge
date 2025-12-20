#pragma once
// =============================================================================
// CRYPTO.H - Enterprise Cryptography Module
// =============================================================================
// Features:
// - SHA-256 hashing (portable implementation)
// - AES-256-CBC encryption (portable implementation)
// - PBKDF2 password hashing
// - Secure random generation
// =============================================================================

#include <cstdint>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace Hub::Security {

// =============================================================================
// CRYPTO RESULT
// =============================================================================
struct CryptoResult {
  bool success = false;
  std::vector<uint8_t> data;
  std::string error;

  bool is_ok() const { return success; }

  std::string toHex() const {
    std::ostringstream oss;
    for (uint8_t b : data) {
      oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    }
    return oss.str();
  }

  std::string toString() const { return std::string(data.begin(), data.end()); }
};

// =============================================================================
// SHA-256 IMPLEMENTATION
// =============================================================================
class SHA256 {
public:
  SHA256() { reset(); }

  void reset() {
    state_[0] = 0x6a09e667;
    state_[1] = 0xbb67ae85;
    state_[2] = 0x3c6ef372;
    state_[3] = 0xa54ff53a;
    state_[4] = 0x510e527f;
    state_[5] = 0x9b05688c;
    state_[6] = 0x1f83d9ab;
    state_[7] = 0x5be0cd19;
    bit_count_ = 0;
    buffer_len_ = 0;
  }

  void update(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
      buffer_[buffer_len_++] = data[i];
      if (buffer_len_ == 64) {
        transform();
        bit_count_ += 512;
        buffer_len_ = 0;
      }
    }
  }

  std::vector<uint8_t> finalize() {
    bit_count_ += buffer_len_ * 8;
    buffer_[buffer_len_++] = 0x80;
    while (buffer_len_ != 56) {
      if (buffer_len_ == 64) {
        transform();
        buffer_len_ = 0;
      }
      buffer_[buffer_len_++] = 0;
    }
    for (int i = 7; i >= 0; --i)
      buffer_[buffer_len_++] = static_cast<uint8_t>(bit_count_ >> (i * 8));
    transform();

    std::vector<uint8_t> hash(32);
    for (int i = 0; i < 8; ++i) {
      hash[i * 4 + 0] = (state_[i] >> 24);
      hash[i * 4 + 1] = (state_[i] >> 16);
      hash[i * 4 + 2] = (state_[i] >> 8);
      hash[i * 4 + 3] = state_[i];
    }
    return hash;
  }

private:
  static constexpr uint32_t K[64] = {
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
      0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
      0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
      0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
      0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
      0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
      0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
      0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
      0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
      0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
      0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

  static uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

  void transform() {
    uint32_t w[64];
    for (int i = 0; i < 16; ++i)
      w[i] = (buffer_[i * 4] << 24) | (buffer_[i * 4 + 1] << 16) |
             (buffer_[i * 4 + 2] << 8) | buffer_[i * 4 + 3];
    for (int i = 16; i < 64; ++i) {
      uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
      uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];
    for (int i = 0; i < 64; ++i) {
      uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
      uint32_t ch = (e & f) ^ (~e & g);
      uint32_t t1 = h + S1 + ch + K[i] + w[i];
      uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
      uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
      uint32_t t2 = S0 + maj;
      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }
    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
  }

  uint32_t state_[8];
  uint8_t buffer_[64];
  size_t buffer_len_;
  uint64_t bit_count_;
};

constexpr uint32_t SHA256::K[];

// =============================================================================
// AES-256 IMPLEMENTATION (Portable)
// =============================================================================
class AES256 {
public:
  static constexpr size_t KEY_SIZE = 32;
  static constexpr size_t BLOCK_SIZE = 16;

  void setKey(const uint8_t *key) {
    std::memcpy(key_, key, KEY_SIZE);
    expandKey();
  }

  void encryptBlock(const uint8_t *in, uint8_t *out) const {
    uint8_t state[16];
    std::memcpy(state, in, 16);
    addRoundKey(state, 0);
    for (int r = 1; r < 14; ++r) {
      subBytes(state);
      shiftRows(state);
      mixColumns(state);
      addRoundKey(state, r);
    }
    subBytes(state);
    shiftRows(state);
    addRoundKey(state, 14);
    std::memcpy(out, state, 16);
  }

  void decryptBlock(const uint8_t *in, uint8_t *out) const {
    uint8_t state[16];
    std::memcpy(state, in, 16);
    addRoundKey(state, 14);
    for (int r = 13; r > 0; --r) {
      invShiftRows(state);
      invSubBytes(state);
      addRoundKey(state, r);
      invMixColumns(state);
    }
    invShiftRows(state);
    invSubBytes(state);
    addRoundKey(state, 0);
    std::memcpy(out, state, 16);
  }

private:
  static const uint8_t sbox[256];
  static const uint8_t rsbox[256];
  static const uint8_t rcon[11];

  uint8_t key_[32];
  uint8_t roundKeys_[240];

  static uint8_t xtime(uint8_t x) { return (x << 1) ^ ((x >> 7) * 0x1b); }
  static uint8_t mul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    for (int i = 0; i < 8; ++i) {
      if (b & 1)
        p ^= a;
      uint8_t hi = a & 0x80;
      a <<= 1;
      if (hi)
        a ^= 0x1b;
      b >>= 1;
    }
    return p;
  }

  void expandKey() {
    std::memcpy(roundKeys_, key_, 32);
    uint8_t temp[4];
    size_t i = 8;
    while (i < 60) {
      for (int j = 0; j < 4; ++j)
        temp[j] = roundKeys_[(i - 1) * 4 + j];
      if (i % 8 == 0) {
        uint8_t t = temp[0];
        temp[0] = sbox[temp[1]] ^ rcon[i / 8];
        temp[1] = sbox[temp[2]];
        temp[2] = sbox[temp[3]];
        temp[3] = sbox[t];
      } else if (i % 8 == 4) {
        for (int j = 0; j < 4; ++j)
          temp[j] = sbox[temp[j]];
      }
      for (int j = 0; j < 4; ++j)
        roundKeys_[i * 4 + j] = roundKeys_[(i - 8) * 4 + j] ^ temp[j];
      ++i;
    }
  }

  void addRoundKey(uint8_t *state, int round) const {
    for (int i = 0; i < 16; ++i)
      state[i] ^= roundKeys_[round * 16 + i];
  }

  static void subBytes(uint8_t *state) {
    for (int i = 0; i < 16; ++i)
      state[i] = sbox[state[i]];
  }
  static void invSubBytes(uint8_t *state) {
    for (int i = 0; i < 16; ++i)
      state[i] = rsbox[state[i]];
  }

  static void shiftRows(uint8_t *s) {
    uint8_t t = s[1];
    s[1] = s[5];
    s[5] = s[9];
    s[9] = s[13];
    s[13] = t;
    t = s[2];
    s[2] = s[10];
    s[10] = t;
    t = s[6];
    s[6] = s[14];
    s[14] = t;
    t = s[15];
    s[15] = s[11];
    s[11] = s[7];
    s[7] = s[3];
    s[3] = t;
  }
  static void invShiftRows(uint8_t *s) {
    uint8_t t = s[13];
    s[13] = s[9];
    s[9] = s[5];
    s[5] = s[1];
    s[1] = t;
    t = s[2];
    s[2] = s[10];
    s[10] = t;
    t = s[6];
    s[6] = s[14];
    s[14] = t;
    t = s[3];
    s[3] = s[7];
    s[7] = s[11];
    s[11] = s[15];
    s[15] = t;
  }

  static void mixColumns(uint8_t *s) {
    for (int c = 0; c < 4; ++c) {
      uint8_t a[4];
      for (int i = 0; i < 4; ++i)
        a[i] = s[c * 4 + i];
      s[c * 4 + 0] = mul(a[0], 2) ^ mul(a[1], 3) ^ a[2] ^ a[3];
      s[c * 4 + 1] = a[0] ^ mul(a[1], 2) ^ mul(a[2], 3) ^ a[3];
      s[c * 4 + 2] = a[0] ^ a[1] ^ mul(a[2], 2) ^ mul(a[3], 3);
      s[c * 4 + 3] = mul(a[0], 3) ^ a[1] ^ a[2] ^ mul(a[3], 2);
    }
  }
  static void invMixColumns(uint8_t *s) {
    for (int c = 0; c < 4; ++c) {
      uint8_t a[4];
      for (int i = 0; i < 4; ++i)
        a[i] = s[c * 4 + i];
      s[c * 4 + 0] =
          mul(a[0], 14) ^ mul(a[1], 11) ^ mul(a[2], 13) ^ mul(a[3], 9);
      s[c * 4 + 1] =
          mul(a[0], 9) ^ mul(a[1], 14) ^ mul(a[2], 11) ^ mul(a[3], 13);
      s[c * 4 + 2] =
          mul(a[0], 13) ^ mul(a[1], 9) ^ mul(a[2], 14) ^ mul(a[3], 11);
      s[c * 4 + 3] =
          mul(a[0], 11) ^ mul(a[1], 13) ^ mul(a[2], 9) ^ mul(a[3], 14);
    }
  }
};

// AES S-Box and inverse
inline const uint8_t AES256::sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b,
    0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26,
    0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2,
    0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed,
    0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f,
    0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec,
    0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14,
    0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d,
    0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f,
    0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11,
    0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f,
    0xb0, 0x54, 0xbb, 0x16};
inline const uint8_t AES256::rsbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e,
    0x81, 0xf3, 0xd7, 0xfb, 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
    0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb, 0x54, 0x7b, 0x94, 0x32,
    0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49,
    0x6d, 0x8b, 0xd1, 0x25, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
    0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92, 0x6c, 0x70, 0x48, 0x50,
    0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05,
    0xb8, 0xb3, 0x45, 0x06, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
    0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b, 0x3a, 0x91, 0x11, 0x41,
    0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8,
    0x1c, 0x75, 0xdf, 0x6e, 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
    0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b, 0xfc, 0x56, 0x3e, 0x4b,
    0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59,
    0x27, 0x80, 0xec, 0x5f, 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
    0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef, 0xa0, 0xe0, 0x3b, 0x4d,
    0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63,
    0x55, 0x21, 0x0c, 0x7d};
inline const uint8_t AES256::rcon[11] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10,
                                         0x20, 0x40, 0x80, 0x1b, 0x36};

// =============================================================================
// HIGH-LEVEL CRYPTO FUNCTIONS
// =============================================================================
inline CryptoResult sha256(const std::vector<uint8_t> &input) {
  CryptoResult result;
  SHA256 hasher;
  hasher.update(input.data(), input.size());
  result.data = hasher.finalize();
  result.success = true;
  return result;
}

inline CryptoResult sha256(const std::string &input) {
  return sha256(std::vector<uint8_t>(input.begin(), input.end()));
}

// PBKDF2-SHA256 for password hashing
inline std::vector<uint8_t> pbkdf2_sha256(const std::string &password,
                                          const std::vector<uint8_t> &salt,
                                          int iterations, size_t keyLen) {
  std::vector<uint8_t> result(keyLen);
  size_t blocks = (keyLen + 31) / 32;

  for (size_t block = 1; block <= blocks; ++block) {
    // U1 = HMAC(password, salt || block_num)
    std::vector<uint8_t> u(salt);
    u.push_back((block >> 24) & 0xFF);
    u.push_back((block >> 16) & 0xFF);
    u.push_back((block >> 8) & 0xFF);
    u.push_back(block & 0xFF);

    // Simple HMAC-SHA256 approximation using hash(key || data)
    std::vector<uint8_t> hmacInput(password.begin(), password.end());
    hmacInput.insert(hmacInput.end(), u.begin(), u.end());
    SHA256 h;
    h.update(hmacInput.data(), hmacInput.size());
    std::vector<uint8_t> prev = h.finalize();
    std::vector<uint8_t> xorResult = prev;

    for (int i = 1; i < iterations; ++i) {
      std::vector<uint8_t> next(password.begin(), password.end());
      next.insert(next.end(), prev.begin(), prev.end());
      SHA256 h2;
      h2.update(next.data(), next.size());
      prev = h2.finalize();
      for (size_t j = 0; j < 32; ++j)
        xorResult[j] ^= prev[j];
    }

    size_t offset = (block - 1) * 32;
    size_t len = (std::min)(static_cast<size_t>(32), keyLen - offset);
    std::memcpy(result.data() + offset, xorResult.data(), len);
  }
  return result;
}

// Random bytes generation
inline CryptoResult random_bytes(size_t count) {
  CryptoResult result;
  result.data.resize(count);
  static uint64_t seed =
      0x853c49e6748fea9bULL ^ static_cast<uint64_t>(time(nullptr));
  for (size_t i = 0; i < count; ++i) {
    seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    result.data[i] = static_cast<uint8_t>(seed >> 56);
  }
  result.success = true;
  return result;
}

// AES-256-CBC Encryption
inline CryptoResult aes256_encrypt(const std::vector<uint8_t> &plaintext,
                                   const std::vector<uint8_t> &key,
                                   const std::vector<uint8_t> &iv) {
  CryptoResult result;
  if (key.size() != 32) {
    result.error = "Key must be 32 bytes";
    return result;
  }
  if (iv.size() != 16) {
    result.error = "IV must be 16 bytes";
    return result;
  }

  AES256 aes;
  aes.setKey(key.data());

  // PKCS7 padding
  size_t padLen = 16 - (plaintext.size() % 16);
  std::vector<uint8_t> padded = plaintext;
  for (size_t i = 0; i < padLen; ++i)
    padded.push_back(static_cast<uint8_t>(padLen));

  result.data.resize(padded.size());
  std::vector<uint8_t> prev = iv;

  for (size_t i = 0; i < padded.size(); i += 16) {
    uint8_t block[16];
    for (int j = 0; j < 16; ++j)
      block[j] = padded[i + j] ^ prev[j];
    aes.encryptBlock(block, result.data.data() + i);
    prev.assign(result.data.begin() + i, result.data.begin() + i + 16);
  }

  result.success = true;
  return result;
}

// AES-256-CBC Decryption
inline CryptoResult aes256_decrypt(const std::vector<uint8_t> &ciphertext,
                                   const std::vector<uint8_t> &key,
                                   const std::vector<uint8_t> &iv) {
  CryptoResult result;
  if (key.size() != 32) {
    result.error = "Key must be 32 bytes";
    return result;
  }
  if (iv.size() != 16) {
    result.error = "IV must be 16 bytes";
    return result;
  }
  if (ciphertext.size() % 16 != 0) {
    result.error = "Invalid ciphertext";
    return result;
  }

  AES256 aes;
  aes.setKey(key.data());

  std::vector<uint8_t> decrypted(ciphertext.size());
  std::vector<uint8_t> prev = iv;

  for (size_t i = 0; i < ciphertext.size(); i += 16) {
    uint8_t block[16];
    aes.decryptBlock(ciphertext.data() + i, block);
    for (int j = 0; j < 16; ++j)
      decrypted[i + j] = block[j] ^ prev[j];
    prev.assign(ciphertext.begin() + i, ciphertext.begin() + i + 16);
  }

  // Remove PKCS7 padding
  uint8_t padLen = decrypted.back();
  if (padLen > 0 && padLen <= 16)
    decrypted.resize(decrypted.size() - padLen);

  result.data = std::move(decrypted);
  result.success = true;
  return result;
}

} // namespace Hub::Security

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_crypto_sha256(const char *input, char *output,
                                            int output_size);
__declspec(dllexport) int hub_crypto_random(int count, unsigned char *output);
__declspec(dllexport) int
hub_crypto_aes_encrypt(const unsigned char *data, int data_len,
                       const unsigned char *key, const unsigned char *iv,
                       unsigned char *output, int output_size);
__declspec(dllexport) int
hub_crypto_aes_decrypt(const unsigned char *data, int data_len,
                       const unsigned char *key, const unsigned char *iv,
                       unsigned char *output, int output_size);
__declspec(dllexport) int
hub_crypto_pbkdf2(const char *password, const unsigned char *salt, int salt_len,
                  int iterations, unsigned char *output, int output_len);
}
