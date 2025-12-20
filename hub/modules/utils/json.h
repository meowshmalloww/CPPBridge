#pragma once
// =============================================================================
// JSON.H - Simple JSON Parser
// =============================================================================
// Lightweight JSON parsing and generation without external dependencies.
// =============================================================================

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace Hub::Utils {

// Forward declaration
class JsonValue;

using JsonNull = std::nullptr_t;
using JsonBool = bool;
using JsonNumber = double;
using JsonString = std::string;
using JsonArray = std::vector<JsonValue>;
using JsonObject = std::map<std::string, JsonValue>;

// =============================================================================
// JSON VALUE
// =============================================================================
class JsonValue {
public:
  using Value =
      std::variant<JsonNull, JsonBool, JsonNumber, JsonString,
                   std::shared_ptr<JsonArray>, std::shared_ptr<JsonObject>>;

  JsonValue() : value_(nullptr) {}
  JsonValue(std::nullptr_t) : value_(nullptr) {}
  JsonValue(bool b) : value_(b) {}
  JsonValue(int n) : value_(static_cast<double>(n)) {}
  JsonValue(double n) : value_(n) {}
  JsonValue(const char *s) : value_(std::string(s)) {}
  JsonValue(const std::string &s) : value_(s) {}
  JsonValue(const JsonArray &arr) : value_(std::make_shared<JsonArray>(arr)) {}
  JsonValue(const JsonObject &obj)
      : value_(std::make_shared<JsonObject>(obj)) {}

  bool isNull() const { return std::holds_alternative<JsonNull>(value_); }
  bool isBool() const { return std::holds_alternative<JsonBool>(value_); }
  bool isNumber() const { return std::holds_alternative<JsonNumber>(value_); }
  bool isString() const { return std::holds_alternative<JsonString>(value_); }
  bool isArray() const {
    return std::holds_alternative<std::shared_ptr<JsonArray>>(value_);
  }
  bool isObject() const {
    return std::holds_alternative<std::shared_ptr<JsonObject>>(value_);
  }

  bool asBool() const { return std::get<JsonBool>(value_); }
  double asNumber() const { return std::get<JsonNumber>(value_); }
  int asInt() const { return static_cast<int>(std::get<JsonNumber>(value_)); }
  const std::string &asString() const { return std::get<JsonString>(value_); }
  JsonArray &asArray() { return *std::get<std::shared_ptr<JsonArray>>(value_); }
  JsonObject &asObject() {
    return *std::get<std::shared_ptr<JsonObject>>(value_);
  }

  // Array/Object access
  JsonValue &operator[](size_t index) { return asArray()[index]; }
  JsonValue &operator[](const std::string &key) { return asObject()[key]; }

  // Stringify
  std::string stringify(bool pretty = false, int indent = 0) const {
    std::ostringstream oss;
    stringify_impl(oss, pretty, indent);
    return oss.str();
  }

private:
  void stringify_impl(std::ostringstream &oss, bool pretty, int indent) const {
    std::string pad(indent * 2, ' ');
    std::string childPad((indent + 1) * 2, ' ');

    if (isNull()) {
      oss << "null";
    } else if (isBool()) {
      oss << (asBool() ? "true" : "false");
    } else if (isNumber()) {
      double n = asNumber();
      if (n == static_cast<int>(n))
        oss << static_cast<int>(n);
      else
        oss << n;
    } else if (isString()) {
      oss << "\"" << escape_string(asString()) << "\"";
    } else if (isArray()) {
      const auto &arr = *std::get<std::shared_ptr<JsonArray>>(value_);
      oss << "[";
      for (size_t i = 0; i < arr.size(); ++i) {
        if (pretty)
          oss << "\n" << childPad;
        arr[i].stringify_impl(oss, pretty, indent + 1);
        if (i < arr.size() - 1)
          oss << ",";
      }
      if (pretty && !arr.empty())
        oss << "\n" << pad;
      oss << "]";
    } else if (isObject()) {
      const auto &obj = *std::get<std::shared_ptr<JsonObject>>(value_);
      oss << "{";
      size_t i = 0;
      for (const auto &[key, val] : obj) {
        if (pretty)
          oss << "\n" << childPad;
        oss << "\"" << key << "\":";
        if (pretty)
          oss << " ";
        val.stringify_impl(oss, pretty, indent + 1);
        if (i++ < obj.size() - 1)
          oss << ",";
      }
      if (pretty && !obj.empty())
        oss << "\n" << pad;
      oss << "}";
    }
  }

  static std::string escape_string(const std::string &s) {
    std::string result;
    for (char c : s) {
      switch (c) {
      case '"':
        result += "\\\"";
        break;
      case '\\':
        result += "\\\\";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        result += c;
      }
    }
    return result;
  }

  Value value_;
};

// =============================================================================
// JSON PARSER
// =============================================================================
class JsonParser {
public:
  static JsonValue parse(const std::string &json) {
    size_t pos = 0;
    return parse_value(json, pos);
  }

private:
  static void skip_whitespace(const std::string &json, size_t &pos) {
    while (pos < json.size() && std::isspace(json[pos]))
      pos++;
  }

  static JsonValue parse_value(const std::string &json, size_t &pos) {
    skip_whitespace(json, pos);
    if (pos >= json.size())
      return nullptr;

    char c = json[pos];
    if (c == 'n')
      return parse_null(json, pos);
    if (c == 't' || c == 'f')
      return parse_bool(json, pos);
    if (c == '"')
      return parse_string(json, pos);
    if (c == '[')
      return parse_array(json, pos);
    if (c == '{')
      return parse_object(json, pos);
    if (c == '-' || std::isdigit(c))
      return parse_number(json, pos);

    return nullptr;
  }

  static JsonValue parse_null(const std::string &json, size_t &pos) {
    if (json.substr(pos, 4) == "null") {
      pos += 4;
      return nullptr;
    }
    return nullptr;
  }

  static JsonValue parse_bool(const std::string &json, size_t &pos) {
    if (json.substr(pos, 4) == "true") {
      pos += 4;
      return true;
    }
    if (json.substr(pos, 5) == "false") {
      pos += 5;
      return false;
    }
    return nullptr;
  }

  static JsonValue parse_number(const std::string &json, size_t &pos) {
    size_t start = pos;
    if (json[pos] == '-')
      pos++;
    while (pos < json.size() &&
           (std::isdigit(json[pos]) || json[pos] == '.' || json[pos] == 'e' ||
            json[pos] == 'E' || json[pos] == '+' || json[pos] == '-')) {
      if ((json[pos] == 'e' || json[pos] == 'E') && pos > start)
        pos++;
      else if (json[pos] == '+' || json[pos] == '-') {
        if (pos > 0 && (json[pos - 1] == 'e' || json[pos - 1] == 'E'))
          pos++;
        else
          break;
      } else
        pos++;
    }
    return std::stod(json.substr(start, pos - start));
  }

  static JsonValue parse_string(const std::string &json, size_t &pos) {
    pos++; // Skip opening quote
    std::string result;
    while (pos < json.size() && json[pos] != '"') {
      if (json[pos] == '\\' && pos + 1 < json.size()) {
        pos++;
        switch (json[pos]) {
        case '"':
          result += '"';
          break;
        case '\\':
          result += '\\';
          break;
        case 'n':
          result += '\n';
          break;
        case 'r':
          result += '\r';
          break;
        case 't':
          result += '\t';
          break;
        default:
          result += json[pos];
        }
      } else {
        result += json[pos];
      }
      pos++;
    }
    pos++; // Skip closing quote
    return result;
  }

  static JsonValue parse_array(const std::string &json, size_t &pos) {
    pos++; // Skip [
    JsonArray arr;
    skip_whitespace(json, pos);
    while (pos < json.size() && json[pos] != ']') {
      arr.push_back(parse_value(json, pos));
      skip_whitespace(json, pos);
      if (json[pos] == ',')
        pos++;
      skip_whitespace(json, pos);
    }
    pos++; // Skip ]
    return arr;
  }

  static JsonValue parse_object(const std::string &json, size_t &pos) {
    pos++; // Skip {
    JsonObject obj;
    skip_whitespace(json, pos);
    while (pos < json.size() && json[pos] != '}') {
      auto key = parse_string(json, pos);
      skip_whitespace(json, pos);
      if (json[pos] == ':')
        pos++;
      skip_whitespace(json, pos);
      obj[key.asString()] = parse_value(json, pos);
      skip_whitespace(json, pos);
      if (json[pos] == ',')
        pos++;
      skip_whitespace(json, pos);
    }
    pos++; // Skip }
    return obj;
  }
};

// Helper function
inline JsonValue parse_json(const std::string &json) {
  return JsonParser::parse(json);
}

} // namespace Hub::Utils

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_json_parse(const char *json, char *output,
                                         int output_size);
__declspec(dllexport) int hub_json_get_string(const char *json, const char *key,
                                              char *output, int output_size);
__declspec(dllexport) double hub_json_get_number(const char *json,
                                                 const char *key);
}
