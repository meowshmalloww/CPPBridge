# 🚀 CPPBridge Quick Start

Get C++ working with JavaScript in **2 minutes**!

## Step 1: Create Your C++ File

Create a file in the `src/` folder (e.g., `src/my_app.cpp`):

```cpp
#include "../cppbridge.h"

// Add EXPOSE() before functions you want in JavaScript
EXPOSE() const char* greet(const char* name) {
    return TEXT("Hello, " + std::string(name) + "!");
}

EXPOSE() int add(int a, int b) {
    return a + b;
}

EXPOSE() const char* getInfo() {
    return JSON("{\"app\": \"MyApp\", \"version\": \"1.0\"}");
}
```

## Step 2: Build

**Windows:** Double-click `build.bat`

**Or use CMake:**
```bash
cmake -B build && cmake --build build
```

## Step 3: Use in JavaScript

```javascript
const cpp = require('./auto-bridge');

// Call your C++ functions directly!
console.log(cpp.greet("World"));     // "Hello, World!"
console.log(cpp.add(5, 3));          // 8
console.log(cpp.getInfo());          // {"app": "MyApp", "version": "1.0"}
console.log(cpp.bridgeTest());       // "✅ CPPBridge is connected!"
```

## ✅ That's It!

---

## Keywords Reference

| Keyword | What It Does | Example |
|---------|--------------|---------|
| `EXPOSE()` | Makes function callable from JS | `EXPOSE() int add(int a, int b)` |
| `TEXT(...)` | Returns a string safely | `return TEXT("Hi " + name);` |
| `JSON(...)` | Returns JSON string | `return JSON("{\"status\": \"ok\"}");` |

## Folder Structure

```
your-project/
├── cppbridge.h          # Include this in your C++ files
├── auto-bridge.js       # Require this in your JS files  
├── build.bat            # Build script
├── backend.dll          # Generated after build
└── src/
    ├── feature1.cpp     # Your C++ code
    ├── feature2.cpp     # More C++ code
    └── utils.cpp        # As many files as you want!
```

## Electron App Example

```javascript
// main.js
const { app, BrowserWindow, ipcMain } = require('electron');
const cpp = require('./auto-bridge');

ipcMain.handle('greet', (e, name) => cpp.greet(name));
ipcMain.handle('add', (e, a, b) => cpp.add(a, b));

// That's it - your Electron app now has C++ power!
```

## React Native Example

The C++ functions are exposed globally via JSI:

```javascript
// App.js
const result = global.greet("World");
console.log(result); // "Hello, World!"
```

---

**Need help?** See [docs/README.md](docs/README.md) for full documentation.
