# Changelog

## [2.0.0] - 2025-12-21

### ✨ Major Features
- **Auto-Bridge System**: Just use `EXPOSE()` keyword - no manual bindings needed!
- **Cross-Platform**: Full support for Windows, macOS, and Linux
- **React Native**: iOS and Android support via JSI
- **WebAssembly**: Browser support via Emscripten

### 📦 New Files
- `cppbridge.h` - Magic one-header include with `EXPOSE()`, `TEXT()`, `JSON()` macros
- `auto-bridge.js` - Auto-discovers and loads C++ functions
- `build.bat` - One-click Windows build script
- `QUICKSTART.md` - 2-minute getting started guide

### 🔧 Improvements
- Simplified API: Just `EXPOSE()` and you're done
- `TEXT()` macro for safe string returns
- `JSON()` macro for JSON data
- Auto-discovery of functions in JavaScript
- Platform guards for React Native JSI code

### 🐛 Fixes
- Fixed thread_local + dllexport conflict on Windows
- Fixed unused include warnings
- Fixed ODR violations in header functions

---

## [1.0.0] - 2025-12-20

### Initial Release
- Core modules: HTTP, Database, KeyValue, Crypto, Compression
- Windows DLL support
- Node.js npm package with ffi-napi
- SQLite database integration
- AES-256 encryption
- LZ77 and Huffman compression
