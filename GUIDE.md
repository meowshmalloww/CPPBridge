# UniversalBridge Complete Guide

**v6.1 Production Edition** - C++ to JavaScript bridge with enterprise features.

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Building](#building)
3. [Functions](#1-functions)
4. [Variables](#2-variables)
5. [Enums](#3-enums)
6. [Structs](#4-structs)
7. [Classes](#5-classes)
8. [Exceptions](#6-exceptions)
9. [Callbacks](#7-callbacks)
10. [Modules](#8-modules)
11. [Auto-Registry](#auto-registry)
12. [Security](#security)

---

## Getting Started

```bash
git clone <repo> UniversalBridge
cd UniversalBridge
npm install
```

---

## Building

### Windows (Recommended)

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
copy build\Release\UniversalBridge.dll bridge\cppbridge.dll
```

**Auto-Build:** GitHub Actions automatically builds Windows binaries on push.

### macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cp build/libUniversalBridge.dylib bridge/cppbridge.dylib
```

### Linux

```bash
sudo apt-get install build-essential cmake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cp build/libUniversalBridge.so bridge/cppbridge.so
```

### After Building

```bash
# Generate registry & TypeScript types
node scripts/generate-registry.js

# Use
const bridge = require('./bridge');
```

---

## 1. Functions

Export any C++ function to JavaScript.

### C++
```cpp
#include "bridge.h"

BRIDGE int add(int a, int b) {
    return a + b;
}

BRIDGE double multiply(double x, double y) {
    return x * y;
}

BRIDGE str greet(str name) {
    return RET("Hello, " + std::string(name) + "!");
}
```

### JavaScript
```javascript
bridge.add(5, 3);        // 8
bridge.multiply(2.5, 4); // 10.0
bridge.greet("World");   // "Hello, World!"
```

---

## 2. Variables

Share variables between C++ and JavaScript with auto-generated getters/setters.

### C++
```cpp
// Integer variable
BRIDGE_VAR(int, counter, 0);

// Double variable
BRIDGE_VAR(double, temperature, 20.0);

// String variable
BRIDGE_VAR_STR(username, "Guest");
```

### JavaScript
```javascript
// Read
bridge.get_counter();      // 0
bridge.get_temperature();  // 20.0
bridge.get_username();     // "Guest"

// Write
bridge.set_counter(42);
bridge.set_temperature(25.5);
bridge.set_username("John");
```

---

## 3. Enums

Export enum values as callable functions.

### C++
```cpp
// 3 values
BRIDGE_ENUM_3(Color, Red, Green, Blue);

// 4 values
BRIDGE_ENUM_4(Status, Idle, Running, Done, Failed);

// 5 values
BRIDGE_ENUM_5(Level, Trace, Debug, Info, Warn, Error);
```

### JavaScript
```javascript
bridge.Color_Red();    // 0
bridge.Color_Green();  // 1
bridge.Color_Blue();   // 2

bridge.Status_Idle();  // 0
bridge.Status_Running(); // 1
```

---

## 4. Structs

Serialize C++ structs to JSON.

### C++
```cpp
// 2 fields
BRIDGE_STRUCT_2(Point, int, x, int, y);

// 3 fields
BRIDGE_STRUCT_3(Vector3, double, x, double, y, double, z);
```

### JavaScript
```javascript
const point = JSON.parse(bridge.Point_create(10, 20));
// { x: 10, y: 20 }

const vec = JSON.parse(bridge.Vector3_create(1.0, 2.0, 3.0));
// { x: 1.0, y: 2.0, z: 3.0 }
```

---

## 5. Classes

Create C++ objects with handles, call methods from JS.

### C++
```cpp
class Calculator {
public:
    int value = 0;
    int getValue() { return value; }
    int addTo(int n) { value += n; return value; }
    void reset() { value = 0; }
};

// Object storage
static std::unordered_map<int, Calculator*> _calcs;
static int _calc_id = 0;

BRIDGE int Calculator_new() {
    int id = ++_calc_id;
    _calcs[id] = new Calculator();
    return id;
}

BRIDGE void Calculator_delete(int handle) {
    if (_calcs.count(handle)) {
        delete _calcs[handle];
        _calcs.erase(handle);
    }
}

BRIDGE int Calculator_getValue(int handle) {
    if (!_calcs.count(handle)) return 0;
    return _calcs[handle]->getValue();
}

BRIDGE int Calculator_addTo(int handle, int n) {
    if (!_calcs.count(handle)) return 0;
    return _calcs[handle]->addTo(n);
}
```

### JavaScript
```javascript
const calc = bridge.Calculator_new();  // Create object
bridge.Calculator_addTo(calc, 10);     // Add 10
bridge.Calculator_getValue(calc);      // 10
bridge.Calculator_delete(calc);        // Cleanup
```

---

## 6. Exceptions

Handle errors safely across the bridge.

### C++
```cpp
BRIDGE_SAFE int divide(int a, int b) {
    if (b == 0) BRIDGE_THROW("Division by zero");
    return a / b;
}

BRIDGE_SAFE double sqrt_safe(double n) {
    if (n < 0) BRIDGE_THROW("Cannot sqrt negative");
    return std::sqrt(n);
}
```

### JavaScript
```javascript
const result = bridge.divide(10, 2);  // 5

bridge.divide(10, 0);  // Returns 0
if (bridge.bridge_has_error()) {
    console.log(bridge.bridge_get_last_error());
    // "Division by zero"
}
```

---

## 7. Callbacks

Send async notifications from C++ to JavaScript.

### C++
```cpp
BRIDGE_CALLBACK(OnProgress, int);
BRIDGE_CALLBACK_STR(OnMessage);

BRIDGE void start_task(int id) {
    call_OnProgress(25);
    call_OnProgress(50);
    call_OnProgress(100);
    call_OnMessage("Complete!");
}
```

### JavaScript
```javascript
// Start task
bridge.start_task(1);

// Poll for callbacks
const callbacks = JSON.parse(bridge.bridge_poll_callbacks());
// [{"callback":"OnProgress","value":25}, ...]

for (const cb of callbacks) {
    if (cb.callback === "OnProgress") {
        console.log("Progress:", cb.value);
    }
}
```

---

## 8. Modules

Group functions by naming convention.

### C++
```cpp
// Math module
BRIDGE double Math_sin(double x) { return std::sin(x); }
BRIDGE double Math_cos(double x) { return std::cos(x); }
BRIDGE double Math_pow(double b, double e) { return std::pow(b, e); }

// String module
BRIDGE int String_length(str s) { return strlen(s); }
BRIDGE str String_upper(str s) {
    std::string r = s;
    for (char& c : r) if (c >= 'a' && c <= 'z') c -= 32;
    return RET(r);
}
```

### JavaScript
```javascript
bridge.Math_sin(0);        // 0
bridge.Math_cos(0);        // 1
bridge.String_length("hi"); // 2
bridge.String_upper("hi"); // "HI"
```

---

## Auto-Registry

Automatically generate registry.json from your C++ code.

```bash
node scripts/generate-registry.js
```

This scans all `.cpp` and `.h` files in `hub/` and generates function signatures.

### Security Features
- ✅ Path traversal protection
- ✅ Function name validation  
- ✅ File size limits (10MB max)
- ✅ Safe JSON generation

---

## Type Reference

| JavaScript | registry.json | C++ |
|------------|---------------|-----|
| `number` | `"int"` | `int` |
| `number` | `"double"` | `double` |
| `string` | `"str"` | `str` or `const char*` |
| `void` | `"void"` | `void` |
| `bigint` | `"uint64"` | `uint64_t` |

---

## Security

UniversalBridge includes security features:

1. **Path Traversal Protection** - Only scans within project
2. **Input Validation** - Function names validated
3. **Safe Returns** - Thread-local storage for strings
4. **Error Handling** - Exceptions caught at boundary

---

## Platform Support

| Platform | Architecture | Status |
|----------|-------------|--------|
| Windows | x64 | ✅ |
| Windows | ARM64 | ✅ |
| macOS | x64, ARM64 | ✅ |
| Linux | x64, ARM64 | ✅ |
