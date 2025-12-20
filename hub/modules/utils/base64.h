#pragma once
// =============================================================================
// BASE64.H - Base64 Encoding/Decoding
// =============================================================================
// Standard Base64 encoding and decoding utilities.
// =============================================================================

#include <cstdint>
#include <string>
#include <vector>

namespace Hub::Utils {

// =============================================================================
// BASE64 ENCODER/DECODER
// =============================================================================
class Base64 {
public:
  static std::string encode(const std::vector<uint8_t> &data) {
    static const char *chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    result.reserve((data.size() + 2) / 3 * 4);

    for (size_t i = 0; i < data.size(); i += 3) {
      uint32_t n = static_cast<uint32_t>(data[i]) << 16;
      if (i + 1 < data.size())
        n |= static_cast<uint32_t>(data[i + 1]) << 8;
      if (i + 2 < data.size())
        n |= data[i + 2];

      result += chars[(n >> 18) & 0x3F];
      result += chars[(n >> 12) & 0x3F];
      result += (i + 1 < data.size()) ? chars[(n >> 6) & 0x3F] : '=';
      result += (i + 2 < data.size()) ? chars[n & 0x3F] : '=';
    }

    return result;
  }

  static std::string encode(const std::string &str) {
    return encode(std::vector<uint8_t>(str.begin(), str.end()));
  }

  static std::vector<uint8_t> decode(const std::string &encoded) {
    static const int table[128] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
        -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1};

    std::vector<uint8_t> result;
    result.reserve(encoded.size() / 4 * 3);

    uint32_t n = 0;
    int bits = 0;

    for (char c : encoded) {
      if (c == '=')
        break;
      if (c < 0 || table[static_cast<int>(c)] == -1)
        continue;

      n = (n << 6) | table[static_cast<int>(c)];
      bits += 6;

      if (bits >= 8) {
        bits -= 8;
        result.push_back(static_cast<uint8_t>((n >> bits) & 0xFF));
      }
    }

    return result;
  }

  static std::string decodeToString(const std::string &encoded) {
    auto data = decode(encoded);
    return std::string(data.begin(), data.end());
  }
};

} // namespace Hub::Utils

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_base64_encode(const unsigned char *data, int len,
                                            char *output, int output_size);
__declspec(dllexport) int
hub_base64_decode(const char *encoded, unsigned char *output, int output_size);
}
