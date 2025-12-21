# Building CPPBridge for WebAssembly

This guide explains how to compile CPPBridge to WebAssembly for use in web browsers.

## Prerequisites

1. **Install Emscripten SDK:**
   ```bash
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh  # Linux/macOS
   # or: emsdk_env.bat     # Windows
   ```

2. **Verify installation:**
   ```bash
   emcc --version
   ```

## Build

```bash
cd UniversalBridge/wasm

# Create build directory
mkdir build && cd build

# Configure with Emscripten
emcmake cmake ..

# Build
emmake make

# Output files:
# - cppbridge.js     (loader)
# - cppbridge.wasm   (WebAssembly binary)
```

## Usage in Browser

### Basic HTML
```html
<!DOCTYPE html>
<html>
<head>
    <script src="cppbridge.js"></script>
</head>
<body>
    <script>
        CPPBridge().then(bridge => {
            // Crypto
            const crypto = new bridge.Crypto();
            console.log('SHA256:', crypto.sha256('hello world'));
            crypto.delete();
            
            // Key-Value Store
            const kv = new bridge.KeyValue();
            const handle = kv.open('/data/settings.db');
            kv.set(handle, 'theme', 'dark');
            console.log('Theme:', kv.get(handle, 'theme'));
            kv.close(handle);
            kv.delete();
        });
    </script>
</body>
</html>
```

### With Module Bundler (Webpack/Vite)
```javascript
import CPPBridge from './cppbridge.js';

async function init() {
    const bridge = await CPPBridge();
    
    const crypto = new bridge.Crypto();
    const hash = crypto.sha256('password123');
    crypto.delete();
    
    return hash;
}
```

## Available Modules

| Module | Methods |
|--------|---------|
| `Crypto` | `sha256`, `base64Encode`, `base64Decode` |
| `KeyValue` | `open`, `set`, `get`, `remove`, `close` |
| `Database` | `open`, `exec`, `query`, `close` |
| `Compression` | `compress`, `decompress` |

## Limitations

- **No HTTP client** - Use browser's `fetch()` API instead
- **No filesystem** - Uses Emscripten's virtual FS (backed by IndexedDB)
- **No process/system** - Browser sandbox restrictions

## File Size

| Build | Size |
|-------|------|
| Debug | ~2MB |
| Release | ~500KB |
| Gzipped | ~150KB |
