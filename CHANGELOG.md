# Changelog

## [6.1.0] - 2025-12-24 - Production Ready

### 🔧 Production Features

#### 1. DLL Hot-Swap (Windows File Lock Fix)
- **Versioned DLL loading** - Loads newest `cppbridge.*.dll` automatically
- No more `EPERM: operation not permitted` during hot reload

#### 2. Zero-Copy Buffers (AI/Video Performance)
- **BRIDGE_BUFFER macro** - Direct memory access for Uint8Array
- No Base64 encoding, no double-copy
- `BRIDGE void process(BRIDGE_BUFFER(data, size))`

#### 3. Auto-Cleanup (Memory Safety)
- **FinalizationRegistry** - C++ objects auto-deleted when JS garbage collects
- No more memory leaks from forgotten `delete()` calls

#### 4. TypeScript Autocomplete
- **bridge.d.ts** auto-generated with all function signatures
- Full VS Code IntelliSense support

### Technical
- **101 functions** bound
- **bridge/index.js** v6.1 with production features

---

## [5.5.0] - Previous Versions

See git history for older changelog entries.
