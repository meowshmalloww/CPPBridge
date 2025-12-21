// =============================================================================
// EXAMPLE: my_functions.cpp
// =============================================================================
// Put this file in the 'src' folder and run build.bat!
// =============================================================================

#include "../cppbridge.h" // Just include this ONE file

// =============================================================================
// YOUR FUNCTIONS - Add EXPOSE() to anything you want in JavaScript
// =============================================================================

// A simple greeting function
EXPOSE() const char *greet(const char *name) {
  return TEXT("Hello, " + std::string(name) + "!");
}

// Add two numbers
EXPOSE() int add(int a, int b) { return a + b; }

// Multiply two numbers
EXPOSE() double multiply(double a, double b) { return a * b; }

// Process some text
EXPOSE() const char *processText(const char *input) {
  std::string result = "Processed: ";
  result += (input ? input : "nothing");
  result += " (length: ";
  result += std::to_string(strlen(input ? input : ""));
  result += ")";
  return TEXT(result);
}

// Return JSON data
EXPOSE() const char *getData() {
  return JSON("{\"status\": \"ok\", \"message\": \"Data from C++!\"}");
}

// =============================================================================
// INTERNAL HELPERS (No EXPOSE = hidden from JavaScript)
// =============================================================================

std::string internalHelper() {
  return "This function is NOT visible to JavaScript";
}

int calculateSomething(int x) { return x * 2 + 1; }
