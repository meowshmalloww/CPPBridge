# 🔧 Building from Source

For developers who want to compile UniversalBridge themselves.

---

## Prerequisites

### Windows
- Visual Studio 2019+ with C++ workload
- CMake 3.16+
- Git

### Linux
```bash
sudo apt install build-essential cmake git
```

### macOS
```bash
xcode-select --install
brew install cmake
```

---

## Clone and Build

```bash
# Clone repository
git clone https://github.com/yourusername/UniversalBridge.git
cd UniversalBridge

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build . --config Release
```

---

## Build Output

After building, you'll find:

| Platform | File | Location |
|----------|------|----------|
| Windows | `UniversalBridge.dll` | `build/Release/` |
| Linux | `libuniversalbridge.so` | `build/` |
| macOS | `libuniversalbridge.dylib` | `build/` |

---

## Running Tests

```bash
# From build directory
ctest --output-on-failure

# Or run directly
./UnitTests        # Linux/macOS
UnitTests.exe      # Windows
```

---

## Project Structure

```
UniversalBridge/
├── hub/
│   ├── core/            # Thread pool, event bus, logging
│   └── modules/
│       ├── database/    # SQLite, ORM, key-value
│       ├── filesystem/  # File I/O, memory mapping
│       ├── network/     # HTTP, WebSocket, FTP
│       ├── security/    # Crypto, sandboxing
│       ├── system/      # Clipboard, registry, process
│       └── utils/       # Compression, caching
├── npm/                 # JavaScript bindings
├── docs/                # Documentation
└── tests/               # Unit tests
```

---

## Adding to npm Package

After building, copy the binary:

```bash
# Windows
cp build/Release/UniversalBridge.dll npm/prebuilds/win32-x64/

# Linux
cp build/libuniversalbridge.so npm/prebuilds/linux-x64/

# macOS Intel
cp build/libuniversalbridge.dylib npm/prebuilds/darwin-x64/

# macOS Apple Silicon
cp build/libuniversalbridge.dylib npm/prebuilds/darwin-arm64/
```

---

## Cross-Compiling

### Linux from Windows (WSL)
```bash
wsl
cd /mnt/c/path/to/UniversalBridge
mkdir build-linux && cd build-linux
cmake .. && make
```

### macOS
Must be built on actual macOS hardware.

---

## Troubleshooting

### CMake can't find compiler
```bash
# Windows: Use Developer Command Prompt
# Linux: Install build-essential
# macOS: Run xcode-select --install
```

### Missing dependencies
```bash
# Windows: Enable "Desktop development with C++" in VS Installer
# Linux: sudo apt install libssl-dev libsqlite3-dev
```

### Link errors
Make sure all `.cpp` files are included in `CMakeLists.txt`.

---

## Contributing

1. Fork the repository
2. Create feature branch: `git checkout -b feature/my-feature`
3. Make changes and add tests
4. Submit pull request

See [CONTRIBUTING.md](./CONTRIBUTING.md) for detailed guidelines.
