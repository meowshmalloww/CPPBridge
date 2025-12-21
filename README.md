# CPPBridge v2.0.0

> **Universal C++ Bridge** - Connect C++ to any frontend with just ONE keyword.

## ✨ Features

- 🚀 **One Keyword**: Just add `EXPOSE()` to any C++ function
- 🖥️ **Electron/Node.js**: Works via koffi FFI
- 📱 **React Native**: iOS & Android via JSI
- 🌐 **WebAssembly**: Browser support via Emscripten
- 🔧 **Zero Config**: Clone, write C++, build, done!

## 🎯 Supported Platforms

| Platform | Status | Method |
|----------|--------|--------|
| Windows | ✅ Ready | DLL + koffi |
| macOS | ✅ Ready | dylib + koffi |
| Linux | ✅ Ready | .so + koffi |
| React Native (iOS) | ✅ Ready | JSI |
| React Native (Android) | ✅ Ready | JSI/JNI |
| Web Browsers | ✅ Ready | WebAssembly |

## 🚀 Quick Start (30 seconds!)

### 1. Clone
```bash
git clone https://github.com/YourRepo/CPPBridge.git
cd CPPBridge
```

### 2. Write C++ (in `src/` folder)
```cpp
#include "../cppbridge.h"

EXPOSE() const char* hello(const char* name) {
    return TEXT("Hello, " + std::string(name));
}

EXPOSE() int add(int a, int b) {
    return a + b;
}
```

### 3. Build
```bash
# Windows: Double-click build.bat
# Or run: cmake -B build && cmake --build build
```

### 4. Use in JavaScript
```javascript
const cpp = require('./auto-bridge');

console.log(cpp.hello("World"));  // "Hello, World"
console.log(cpp.add(5, 3));       // 8
```

**That's it!** No complex FFI code. No manual bindings. Just `EXPOSE()`.

## 📁 Project Structure

```
CPPBridge/
├── cppbridge.h       # Magic header (just include this)
├── auto-bridge.js    # Auto-discovers your functions
├── build.bat         # One-click Windows build
├── src/              # Put your C++ files here
│   └── my_code.cpp   # Your custom functions
├── hub/              # Core library modules
├── npm/              # Node.js package
├── react-native/     # React Native module
└── wasm/             # WebAssembly build
```

## 📖 Documentation

- [QUICKSTART.md](QUICKSTART.md) - Get started in 2 minutes
- [docs/README.md](docs/README.md) - Full documentation
- [docs/API.md](docs/API.md) - API reference

## 🔑 Keywords

| Keyword | Purpose |
|---------|---------|
| `EXPOSE()` | Makes function callable from JavaScript |
| `TEXT(...)` | Safely returns dynamic strings |
| `JSON(...)` | Returns JSON strings |

## 📜 License

MIT License - Use freely in personal and commercial projects.
