# UniversalBridge Complete Guide

**v6.1 Production Edition** - The easiest way to bridge C++ to JavaScript.

---

## Quick Start

```bash
git clone <repo> UniversalBridge
cd UniversalBridge
npm install
```

---

## Building

### Windows

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
copy build\Release\UniversalBridge.dll bridge\cppbridge.dll
node scripts/generate-registry.js
```

### macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cp build/libUniversalBridge.dylib bridge/cppbridge.dylib
node scripts/generate-registry.js
```

### Linux

```bash
sudo apt-get install build-essential cmake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cp build/libUniversalBridge.so bridge/cppbridge.so
node scripts/generate-registry.js
```

---

## How Easy Is It?

### Add 1 Line in C++ → Use in JavaScript!

**C++ (hub/example_bridge.cpp):**
```cpp
#include "bridge.h"

BRIDGE int add(int a, int b) {
    return a + b;
}
```

**JavaScript:**
```javascript
const bridge = require('./bridge');
bridge.add(5, 3);  // 8
```

---

## All Keywords

| Keyword | C++ Example | What It Does |
|---------|-------------|--------------|
| `BRIDGE` | `BRIDGE int foo()` | Export function |
| `BRIDGE_VAR` | `BRIDGE_VAR(int, count, 0)` | Shared variable |
| `BRIDGE_ENUM_N` | `BRIDGE_ENUM_3(Color, R, G, B)` | Export enum |
| `BRIDGE_STRUCT_N` | `BRIDGE_STRUCT_2(Point, int, x, int, y)` | Struct → JSON |
| `BRIDGE_SAFE` | `BRIDGE_SAFE int divide(...)` | With error handling |
| `BRIDGE_THROW` | `BRIDGE_THROW("Error!")` | Throw to JS |

---

## Examples

### Variables
```cpp
BRIDGE_VAR(int, counter, 0);
BRIDGE_VAR_STR(username, "Guest");
```
```javascript
bridge.get_counter();       // 0
bridge.set_counter(42);
bridge.get_username();      // "Guest"
```

### Enums
```cpp
BRIDGE_ENUM_3(Color, Red, Green, Blue);
```
```javascript
bridge.Color_Red();    // 0
bridge.Color_Green();  // 1
bridge.Color_Blue();   // 2
```

### Structs
```cpp
BRIDGE_STRUCT_2(Point, int, x, int, y);
```
```javascript
JSON.parse(bridge.Point_create(10, 20));  // {x: 10, y: 20}
```

### Classes
```cpp
// In C++
class Calculator {
    int value = 0;
public:
    void add(int x) { value += x; }
    int get() { return value; }
};
```
```javascript
// Auto-cleanup with FinalizationRegistry
const Factory = bridge.createClass("Calculator");
const calc = Factory.create();
calc.add(10);
// Auto-deleted when garbage collected!
```

---

## License

MIT
