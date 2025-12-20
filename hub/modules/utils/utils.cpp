// =============================================================================
// UTILS.CPP - Utility Functions Implementation
// =============================================================================

#include "base64.h"
#include "json.h"
#include <cstring>


static thread_local std::string tl_result;

extern "C" {

// =============================================================================
// JSON FUNCTIONS
// =============================================================================
__declspec(dllexport) int hub_json_parse(const char *json, char *output,
                                         int output_size) {
  if (!json)
    return -1;

  try {
    auto parsed = Hub::Utils::parse_json(json);
    tl_result = parsed.stringify(true);

    if (output && output_size > 0) {
      size_t copy_size =
          (std::min)(static_cast<size_t>(output_size - 1), tl_result.size());
      std::memcpy(output, tl_result.data(), copy_size);
      output[copy_size] = '\0';
    }

    return static_cast<int>(tl_result.size());
  } catch (...) {
    return -1;
  }
}

__declspec(dllexport) int hub_json_get_string(const char *json, const char *key,
                                              char *output, int output_size) {
  if (!json || !key)
    return -1;

  try {
    auto parsed = Hub::Utils::parse_json(json);
    if (!parsed.isObject())
      return -1;

    auto &obj = parsed.asObject();
    auto it = obj.find(key);
    if (it == obj.end() || !it->second.isString())
      return -1;

    tl_result = it->second.asString();

    if (output && output_size > 0) {
      size_t copy_size =
          (std::min)(static_cast<size_t>(output_size - 1), tl_result.size());
      std::memcpy(output, tl_result.data(), copy_size);
      output[copy_size] = '\0';
    }

    return static_cast<int>(tl_result.size());
  } catch (...) {
    return -1;
  }
}

__declspec(dllexport) double hub_json_get_number(const char *json,
                                                 const char *key) {
  if (!json || !key)
    return 0;

  try {
    auto parsed = Hub::Utils::parse_json(json);
    if (!parsed.isObject())
      return 0;

    auto &obj = parsed.asObject();
    auto it = obj.find(key);
    if (it == obj.end() || !it->second.isNumber())
      return 0;

    return it->second.asNumber();
  } catch (...) {
    return 0;
  }
}

// =============================================================================
// BASE64 FUNCTIONS
// =============================================================================
__declspec(dllexport) int hub_base64_encode(const unsigned char *data, int len,
                                            char *output, int output_size) {
  if (!data || len <= 0)
    return -1;

  std::vector<uint8_t> input(data, data + len);
  tl_result = Hub::Utils::Base64::encode(input);

  if (output && output_size > 0) {
    size_t copy_size =
        (std::min)(static_cast<size_t>(output_size - 1), tl_result.size());
    std::memcpy(output, tl_result.data(), copy_size);
    output[copy_size] = '\0';
  }

  return static_cast<int>(tl_result.size());
}

__declspec(dllexport) int
hub_base64_decode(const char *encoded, unsigned char *output, int output_size) {
  if (!encoded)
    return -1;

  auto decoded = Hub::Utils::Base64::decode(encoded);

  if (output && output_size > 0) {
    size_t copy_size =
        (std::min)(static_cast<size_t>(output_size), decoded.size());
    std::memcpy(output, decoded.data(), copy_size);
  }

  return static_cast<int>(decoded.size());
}

} // extern "C"
