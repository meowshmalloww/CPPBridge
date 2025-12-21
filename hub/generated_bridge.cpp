// =============================================================================
// GENERATED_BRIDGE.CPP - Example Backend Functions
// =============================================================================
// This file contains additional exported functions for the DLL.
// The basic functions (bridgeTest, bridgeVersion, bridgeEcho) are in
// cppbridge.h
// =============================================================================

#include "backend_export.h"
#include <ctime>
#include <sstream>

// =============================================================================
// ADDITIONAL BACKEND FUNCTIONS
// =============================================================================

BACKEND_API const char *backendMessage() {
  RETURN_STRING("✅ Backend is connected and working!");
}

BACKEND_API double multiply(double a, double b) { return a * b; }

BACKEND_API const char *getTimestamp() {
  time_t now = time(nullptr);
  std::string ts = std::to_string(now);
  RETURN_STRING(ts);
}

BACKEND_API const char *processData(const char *input, int length) {
  std::ostringstream oss;
  oss << "Processed " << length << " bytes: " << (input ? input : "null");
  RETURN_STRING(oss.str());
}

// =============================================================================
// INITIALIZATION (Called when library loads)
// =============================================================================

#ifdef BACKEND_PLATFORM_WINDOWS
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
  (void)hModule;
  (void)lpReserved;
  switch (reason) {
  case DLL_PROCESS_ATTACH:
    break;
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}
#endif
