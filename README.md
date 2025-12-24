# UniversalBridge v6.1 - C++ Backend for JS/TS Apps

**Production-ready C++ to JavaScript bridge with enterprise features.**

## Features

| Feature | Description |
|---------|-------------|
| **Functions** | Export any C++ function to JS |
| **Variables** | Shared variables with auto getter/setter |
| **Enums** | Export enum values |
| **Structs** | Serialize to JSON |
| **Classes** | Object handles with auto-cleanup |
| **Exceptions** | Error handling across bridge |
| **Callbacks** | Push & poll patterns |
| **TypeScript** | Full autocomplete (bridge.d.ts) |
| **Hot-Swap** | DLL versioning for Windows |
| **Zero-Copy** | Direct buffer access for AI/Video |

---

## Quick Start

```bash
# Clone & install
git clone <repo> UniversalBridge
cd UniversalBridge
npm install

# Build
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
copy build\Release\UniversalBridge.dll bridge\cppbridge.dll

# Generate registry & types
node scripts/generate-registry.js
```

---

## Write C++ Code

```cpp
#include "bridge.h"

// Functions
BRIDGE int add(int a, int b) { return a + b; }

// Variables
BRIDGE_VAR(int, counter, 0);

// Enums
BRIDGE_ENUM_3(Color, Red, Green, Blue);

// Structs
BRIDGE_STRUCT_2(Point, int, x, int, y);

// Zero-Copy Buffers (AI/Video)
BRIDGE void processImage(BRIDGE_BUFFER(data, size)) {
    for (int i = 0; i < size; i++) data[i] = 255 - data[i];
}
```

---

## Use from JavaScript

```javascript
const bridge = require('./bridge');

// Functions
bridge.add(5, 3);           // 8

// Variables
bridge.set_counter(42);
bridge.get_counter();       // 42

// Enums
bridge.Color_Red();         // 0

// Structs
JSON.parse(bridge.Point_create(10, 20));  // {x:10, y:20}

// Classes (with auto-cleanup!)
const Factory = bridge.createClass("DemoCalc");
const calc = Factory.create();  // Auto-deletes on GC!
```

---

## All Keywords

| Keyword | Purpose |
|---------|---------|
| `BRIDGE` | Export function |
| `BRIDGE_VAR` | Shared variable |
| `BRIDGE_VAR_STR` | Shared string |
| `BRIDGE_ENUM_N` | Export enum |
| `BRIDGE_STRUCT_N` | Struct to JSON |
| `BRIDGE_SAFE` | Safe function |
| `BRIDGE_THROW` | Throw error |
| `BRIDGE_CALLBACK` | Async callback |
| `BRIDGE_BUFFER` | Zero-copy buffer |
| `str` / `RET(x)` | String helpers |

---

## Documentation

- [GUIDE.md](GUIDE.md) - Complete tutorial
- [CHANGELOG.md](CHANGELOG.md) - Version history

## License

MIT
