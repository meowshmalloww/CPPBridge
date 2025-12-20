// =============================================================================
// COMPRESS_ADV.CPP - Advanced Compression Implementation
// =============================================================================

#include "compress_adv.h"

extern "C" {

__declspec(dllexport) int hub_huffman_compress(const unsigned char *data,
                                               int len, unsigned char *output,
                                               int output_size) {

  if (!data || len <= 0 || !output)
    return -1;

  std::vector<uint8_t> input(data, data + len);
  auto compressed = Hub::Compress::Huffman::compress(input);

  if (compressed.size() > static_cast<size_t>(output_size)) {
    return static_cast<int>(compressed.size()); // Return needed size
  }

  std::memcpy(output, compressed.data(), compressed.size());
  return static_cast<int>(compressed.size());
}

__declspec(dllexport) int hub_huffman_decompress(const unsigned char *data,
                                                 int len, unsigned char *output,
                                                 int output_size) {

  if (!data || len <= 0 || !output)
    return -1;

  std::vector<uint8_t> input(data, data + len);
  auto decompressed = Hub::Compress::Huffman::decompress(input);

  if (decompressed.empty())
    return -1;
  if (decompressed.size() > static_cast<size_t>(output_size)) {
    return static_cast<int>(decompressed.size());
  }

  std::memcpy(output, decompressed.data(), decompressed.size());
  return static_cast<int>(decompressed.size());
}

__declspec(dllexport) int hub_zlib_compress(const unsigned char *data, int len,
                                            unsigned char *output,
                                            int output_size) {

  if (!data || len <= 0 || !output)
    return -1;

  std::vector<uint8_t> input(data, data + len);
  auto compressed = Hub::Compress::Deflate::compress(input);

  if (compressed.size() > static_cast<size_t>(output_size)) {
    return static_cast<int>(compressed.size());
  }

  std::memcpy(output, compressed.data(), compressed.size());
  return static_cast<int>(compressed.size());
}

__declspec(dllexport) int hub_zlib_decompress(const unsigned char *data,
                                              int len, unsigned char *output,
                                              int output_size) {

  if (!data || len <= 0 || !output)
    return -1;

  std::vector<uint8_t> input(data, data + len);
  auto decompressed = Hub::Compress::Deflate::decompress(input);

  if (decompressed.empty())
    return -1;
  if (decompressed.size() > static_cast<size_t>(output_size)) {
    return static_cast<int>(decompressed.size());
  }

  std::memcpy(output, decompressed.data(), decompressed.size());
  return static_cast<int>(decompressed.size());
}

} // extern "C"
