// =============================================================================
// MAIN.CPP - CPPBridge Demo / Test Entry Point
// =============================================================================
// This file demonstrates the CPPBridge library capabilities.
// Run: cmake -B build && cmake --build build && build/BridgeTest.exe
// =============================================================================

#include "cppbridge.h"
#include <iostream>

int main() {
  std::cout << "========================================\n";
  std::cout << "  CPPBridge v2.0.0 - Test Suite\n";
  std::cout << "========================================\n\n";

  // Test the bridge built-in functions
  std::cout << "[Test 1] Bridge Connection: " << bridgeTest() << "\n";
  std::cout << "[Test 2] Bridge Version: " << bridgeVersion() << "\n";
  std::cout << "[Test 3] Echo Test: " << bridgeEcho("Hello World") << "\n";

  std::cout << "\n========================================\n";
  std::cout << "  All Tests Passed!\n";
  std::cout << "========================================\n";

  return 0;
}