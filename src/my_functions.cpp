// =============================================================================
// EXAMPLE: src/my_functions.cpp - CPPBridge v3.1
// =============================================================================
// Put this file in the 'src' folder and run: npm run build:bridge
// =============================================================================

#include "../bridge_core.h"

// =============================================================================
// MATH FUNCTIONS
// =============================================================================

BRIDGE_FN(int, add, int a, int b) { return a + b; }

BRIDGE_FN(int, subtract, int a, int b) { return a - b; }

BRIDGE_FN(double, multiply, double x, double y) { return x * y; }

BRIDGE_FN(int, divide, int a, int b) {
  // Manual crash protection
  if (b == 0) {
    std::cerr << "[CPPBridge] divide: Cannot divide by zero" << std::endl;
    return 0;
  }
  return a / b;
}

// =============================================================================
// STRING FUNCTIONS
// =============================================================================

BRIDGE_FN(const char *, greet, const char *name) {
  return BRIDGE_STRING("Hello, " + std::string(name ? name : "World") + "!");
}

BRIDGE_FN(const char *, getVersion) { return "3.1.0"; }

BRIDGE_FN(const char *, getAppInfo) {
  return BRIDGE_STRING("{\"name\": \"CPPBridge\", \"version\": \"3.1.0\"}");
}

BRIDGE_FN(const char *, processText, const char *input) {
  std::string text = input ? input : "";
  std::string result =
      "Processed (" + std::to_string(text.length()) + " chars): " + text;
  return BRIDGE_STRING(result);
}

BRIDGE_FN(int, stringLength, const char *str) {
  return str ? static_cast<int>(strlen(str)) : 0;
}

// =============================================================================
// TEST FUNCTION
// =============================================================================

BRIDGE_FN(const char *, bridgeTest) { return "✅ CPPBridge v3.1 is working!"; }
