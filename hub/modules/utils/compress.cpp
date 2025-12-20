// =============================================================================
// COMPRESS.CPP - Compression Implementation
// =============================================================================

#include "compress.h"
#include <algorithm>

extern "C" {

__declspec(dllexport) int hub_compress(const unsigned char *data, int len,
                                       unsigned char *output, int output_size) {
  if (!data || len <= 0 || !output)
    return -1;

  std::vector<uint8_t> input(data, data + len);
  auto result = Hub::Utils::LZ::compress(input);

  if (!result.success)
    return -1;

  int needed = static_cast<int>(result.data.size());
  if (output_size < needed)
    return needed;

  std::copy(result.data.begin(), result.data.end(), output);
  return needed;
}

__declspec(dllexport) int hub_decompress(const unsigned char *data, int len,
                                         unsigned char *output,
                                         int output_size) {
  if (!data || len <= 0 || !output)
    return -1;

  std::vector<uint8_t> input(data, data + len);
  auto result = Hub::Utils::LZ::decompress(input);

  if (!result.success)
    return -1;

  int needed = static_cast<int>(result.data.size());
  if (output_size < needed)
    return needed;

  std::copy(result.data.begin(), result.data.end(), output);
  return needed;
}

__declspec(dllexport) int hub_compress_rle(const unsigned char *data, int len,
                                           unsigned char *output,
                                           int output_size) {
  if (!data || len <= 0 || !output)
    return -1;

  std::vector<uint8_t> input(data, data + len);
  auto result = Hub::Utils::RLE::compress(input);

  if (!result.success)
    return -1;

  int needed = static_cast<int>(result.data.size());
  if (output_size < needed)
    return needed;

  std::copy(result.data.begin(), result.data.end(), output);
  return needed;
}

__declspec(dllexport) int hub_decompress_rle(const unsigned char *data, int len,
                                             unsigned char *output,
                                             int output_size) {
  if (!data || len <= 0 || !output)
    return -1;

  std::vector<uint8_t> input(data, data + len);
  auto result = Hub::Utils::RLE::decompress(input);

  if (!result.success)
    return -1;

  int needed = static_cast<int>(result.data.size());
  if (output_size < needed)
    return needed;

  std::copy(result.data.begin(), result.data.end(), output);
  return needed;
}

} // extern "C"
