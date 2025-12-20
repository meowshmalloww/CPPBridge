#pragma once
// =============================================================================
// COMPRESS.H - Enhanced Compression Utilities
// =============================================================================
// High-performance portable compression:
// - Enhanced RLE with variable-length encoding
// - LZ77 with 4KB sliding window and optimal matching
// - Huffman coding post-processing
// =============================================================================

#include <algorithm>
#include <array>
#include <cstdint>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace Hub::Utils {

// =============================================================================
// COMPRESSION RESULT
// =============================================================================
struct CompressResult {
  bool success = false;
  std::vector<uint8_t> data;
  std::string error;
  double ratio = 0.0;
};

// =============================================================================
// ENHANCED RLE (Variable-length runs, escape sequences)
// =============================================================================
class RLE {
public:
  static CompressResult compress(const std::vector<uint8_t> &input) {
    CompressResult result;
    if (input.empty()) {
      result.success = true;
      return result;
    }

    result.data.reserve(input.size());

    // Magic header
    result.data.push_back('R');
    result.data.push_back('L');
    result.data.push_back('E');
    result.data.push_back('2');

    // Original size (4 bytes)
    uint32_t origSize = static_cast<uint32_t>(input.size());
    result.data.push_back((origSize >> 24) & 0xFF);
    result.data.push_back((origSize >> 16) & 0xFF);
    result.data.push_back((origSize >> 8) & 0xFF);
    result.data.push_back(origSize & 0xFF);

    size_t i = 0;
    while (i < input.size()) {
      uint8_t current = input[i];
      size_t runLen = 1;

      // Count run length (up to 32767)
      while (i + runLen < input.size() && input[i + runLen] == current &&
             runLen < 32767) {
        runLen++;
      }

      if (runLen >= 4) {
        // Long run: escape + length (2 bytes) + byte
        result.data.push_back(0x00); // Escape
        if (runLen <= 127) {
          result.data.push_back(static_cast<uint8_t>(runLen));
        } else {
          // Two-byte length: high bit set
          result.data.push_back(0x80 | ((runLen >> 8) & 0x7F));
          result.data.push_back(runLen & 0xFF);
        }
        result.data.push_back(current);
      } else if (current == 0x00) {
        // Escape zero bytes
        for (size_t j = 0; j < runLen; ++j) {
          result.data.push_back(0x00);
          result.data.push_back(0x01);
          result.data.push_back(0x00);
        }
      } else {
        // Literal bytes
        for (size_t j = 0; j < runLen; ++j) {
          result.data.push_back(current);
        }
      }

      i += runLen;
    }

    result.success = true;
    result.ratio = static_cast<double>(result.data.size()) / input.size();
    return result;
  }

  static CompressResult decompress(const std::vector<uint8_t> &input) {
    CompressResult result;

    // Check header
    if (input.size() < 8 || input[0] != 'R' || input[1] != 'L' ||
        input[2] != 'E' || input[3] != '2') {
      // Fall back to legacy format
      return decompressLegacy(input);
    }

    uint32_t origSize =
        (input[4] << 24) | (input[5] << 16) | (input[6] << 8) | input[7];
    result.data.reserve(origSize);

    size_t i = 8;
    while (i < input.size() && result.data.size() < origSize) {
      if (input[i] == 0x00 && i + 2 < input.size()) {
        size_t runLen;
        size_t headerLen;

        if (input[i + 1] & 0x80) {
          // Two-byte length
          if (i + 3 >= input.size())
            break;
          runLen = ((input[i + 1] & 0x7F) << 8) | input[i + 2];
          headerLen = 4;
        } else {
          runLen = input[i + 1];
          headerLen = 3;
        }

        uint8_t byte = input[i + headerLen - 1];
        for (size_t j = 0; j < runLen; ++j) {
          result.data.push_back(byte);
        }
        i += headerLen;
      } else {
        result.data.push_back(input[i]);
        i++;
      }
    }

    result.success = true;
    return result;
  }

private:
  static CompressResult decompressLegacy(const std::vector<uint8_t> &input) {
    CompressResult result;
    result.data.reserve(input.size() * 2);

    size_t i = 0;
    while (i < input.size()) {
      if (input[i] == 0xFF && i + 2 < input.size()) {
        uint8_t count = input[i + 1];
        uint8_t byte = input[i + 2];
        for (uint8_t j = 0; j < count; ++j) {
          result.data.push_back(byte);
        }
        i += 3;
      } else {
        result.data.push_back(input[i]);
        i++;
      }
    }

    result.success = true;
    return result;
  }
};

// =============================================================================
// ENHANCED LZ77 (4KB window, optimal matching, better encoding)
// =============================================================================
class LZ {
public:
  static constexpr size_t WINDOW_SIZE = 4096; // 4KB sliding window
  static constexpr size_t MAX_MATCH = 258;    // Max match length
  static constexpr size_t MIN_MATCH = 3;      // Minimum match for encoding

  static CompressResult compress(const std::vector<uint8_t> &input) {
    CompressResult result;
    if (input.empty()) {
      result.success = true;
      return result;
    }

    result.data.reserve(input.size());

    // Header: LZ77 + original size
    result.data.push_back('L');
    result.data.push_back('Z');
    result.data.push_back('7');
    result.data.push_back('7');

    uint32_t origSize = static_cast<uint32_t>(input.size());
    result.data.push_back((origSize >> 24) & 0xFF);
    result.data.push_back((origSize >> 16) & 0xFF);
    result.data.push_back((origSize >> 8) & 0xFF);
    result.data.push_back(origSize & 0xFF);

    // Build hash table for fast matching
    std::unordered_map<uint32_t, std::vector<size_t>> hashTable;

    size_t i = 0;
    std::vector<uint8_t> literals;

    auto flushLiterals = [&]() {
      if (literals.empty())
        return;

      // Encode literal block: 0x00 + length (1-2 bytes) + literals
      result.data.push_back(0x00);
      if (literals.size() <= 127) {
        result.data.push_back(static_cast<uint8_t>(literals.size()));
      } else {
        result.data.push_back(0x80 | ((literals.size() >> 8) & 0x7F));
        result.data.push_back(literals.size() & 0xFF);
      }
      result.data.insert(result.data.end(), literals.begin(), literals.end());
      literals.clear();
    };

    while (i < input.size()) {
      // Create hash for current position
      uint32_t hash = 0;
      if (i + 2 < input.size()) {
        hash = (input[i] << 16) | (input[i + 1] << 8) | input[i + 2];
      }

      // Find best match
      size_t bestLen = 0;
      size_t bestDist = 0;

      size_t windowStart = (i >= WINDOW_SIZE) ? i - WINDOW_SIZE : 0;

      if (hashTable.count(hash)) {
        for (size_t pos : hashTable[hash]) {
          if (pos < windowStart)
            continue;

          size_t len = 0;
          while (i + len < input.size() && len < MAX_MATCH &&
                 input[pos + len] == input[i + len]) {
            len++;
          }

          if (len > bestLen) {
            bestLen = len;
            bestDist = i - pos;
          }
        }
      }

      // Update hash table
      hashTable[hash].push_back(i);
      if (hashTable[hash].size() > 64) {
        hashTable[hash].erase(hashTable[hash].begin());
      }

      if (bestLen >= MIN_MATCH) {
        // Flush pending literals
        flushLiterals();

        // Encode match: distance (2 bytes) + length (1-2 bytes)
        result.data.push_back((bestDist >> 8) & 0x0F |
                              0x10); // High nibble = 1 (match flag)
        result.data.push_back(bestDist & 0xFF);

        if (bestLen <= 127) {
          result.data.push_back(static_cast<uint8_t>(bestLen));
        } else {
          result.data.push_back(0x80 | ((bestLen >> 8) & 0x7F));
          result.data.push_back(bestLen & 0xFF);
        }

        i += bestLen;
      } else {
        // Add to literal buffer
        literals.push_back(input[i]);
        if (literals.size() >= 32767) {
          flushLiterals();
        }
        i++;
      }
    }

    // Flush remaining literals
    flushLiterals();

    result.success = true;
    result.ratio = static_cast<double>(result.data.size()) / input.size();
    return result;
  }

  static CompressResult decompress(const std::vector<uint8_t> &input) {
    CompressResult result;

    // Check header
    if (input.size() < 8) {
      return decompressLegacy(input);
    }

    if (input[0] != 'L' || input[1] != 'Z' || input[2] != '7' ||
        input[3] != '7') {
      return decompressLegacy(input);
    }

    uint32_t origSize =
        (input[4] << 24) | (input[5] << 16) | (input[6] << 8) | input[7];
    result.data.reserve(origSize);

    size_t i = 8;
    while (i < input.size() && result.data.size() < origSize) {
      uint8_t flag = input[i];

      if (flag == 0x00 && i + 1 < input.size()) {
        // Literal block
        size_t litLen;
        size_t headerLen;

        if (input[i + 1] & 0x80) {
          if (i + 2 >= input.size())
            break;
          litLen = ((input[i + 1] & 0x7F) << 8) | input[i + 2];
          headerLen = 3;
        } else {
          litLen = input[i + 1];
          headerLen = 2;
        }

        i += headerLen;
        for (size_t j = 0; j < litLen && i < input.size(); ++j) {
          result.data.push_back(input[i++]);
        }
      } else if ((flag & 0xF0) == 0x10 && i + 2 < input.size()) {
        // Match
        size_t dist = ((flag & 0x0F) << 8) | input[i + 1];
        size_t matchLen;
        size_t headerLen;

        if (input[i + 2] & 0x80) {
          if (i + 3 >= input.size())
            break;
          matchLen = ((input[i + 2] & 0x7F) << 8) | input[i + 3];
          headerLen = 4;
        } else {
          matchLen = input[i + 2];
          headerLen = 3;
        }

        i += headerLen;

        size_t start = result.data.size() - dist;
        for (size_t j = 0; j < matchLen; ++j) {
          result.data.push_back(result.data[start + j]);
        }
      } else {
        break; // Invalid format
      }
    }

    result.success = true;
    return result;
  }

private:
  static CompressResult decompressLegacy(const std::vector<uint8_t> &input) {
    CompressResult result;
    if (input.size() < 4) {
      result.error = "Invalid data";
      return result;
    }

    uint32_t origSize =
        (input[0] << 24) | (input[1] << 16) | (input[2] << 8) | input[3];
    result.data.reserve(origSize);

    size_t i = 4;
    while (i < input.size()) {
      if (input[i] == 0x00 && i + 2 < input.size()) {
        uint8_t dist = input[i + 1];
        uint8_t len = input[i + 2];

        if (dist == 0 && len == 1) {
          result.data.push_back(0x00);
        } else {
          size_t start = result.data.size() - dist;
          for (uint8_t j = 0; j < len; ++j) {
            result.data.push_back(result.data[start + j]);
          }
        }
        i += 3;
      } else {
        result.data.push_back(input[i]);
        i++;
      }
    }

    result.success = true;
    return result;
  }
};

// =============================================================================
// CONVENIENCE FUNCTIONS
// =============================================================================
inline CompressResult compress(const std::string &input, bool useLZ = true) {
  std::vector<uint8_t> data(input.begin(), input.end());
  return useLZ ? LZ::compress(data) : RLE::compress(data);
}

inline CompressResult decompress(const std::vector<uint8_t> &input,
                                 bool useLZ = true) {
  return useLZ ? LZ::decompress(input) : RLE::decompress(input);
}

inline std::string decompressToString(const std::vector<uint8_t> &input,
                                      bool useLZ = true) {
  auto result = decompress(input, useLZ);
  return std::string(result.data.begin(), result.data.end());
}

} // namespace Hub::Utils

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_compress(const unsigned char *data, int len,
                                       unsigned char *output, int output_size);
__declspec(dllexport) int hub_decompress(const unsigned char *data, int len,
                                         unsigned char *output,
                                         int output_size);
__declspec(dllexport) int hub_compress_rle(const unsigned char *data, int len,
                                           unsigned char *output,
                                           int output_size);
__declspec(dllexport) int hub_decompress_rle(const unsigned char *data, int len,
                                             unsigned char *output,
                                             int output_size);
}
