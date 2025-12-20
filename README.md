# UniversalBridge

🚀 **Native C++ backend for React Native, Electron, and Node.js applications.**

[![Version](https://img.shields.io/badge/version-1.0.0-green.svg)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blue)]()

## Features

| Category | Features |
|----------|----------|
| 🔒 **Security** | AES-256-GCM, SHA-256, PBKDF2, Path Sandboxing |
| 🌐 **Network** | HTTP with retry, WebSocket with auto-reconnect |
| 💾 **Storage** | SQLite, Key-Value Store, Memory-Mapped Files |
| 🖥️ **System** | Process Management, Registry, Clipboard |
| 📝 **Logging** | Async JSON logging with rotation |

## Quick Start

```bash
npm install universalbridge
```

```javascript
import { http, database, keyvalue } from 'universalbridge';

// HTTP request
const response = http.get('https://api.github.com');

// Key-Value storage
const store = keyvalue.open('settings.db');
store.set('theme', 'dark');
console.log(store.get('theme')); // 'dark'

// Database
const db = database.open('app.db');
db.exec('CREATE TABLE users (id INTEGER, name TEXT)');
```

## Documentation

- 📖 [Quickstart Guide](docs/QUICKSTART.md) - Get running in 5 minutes
- 📚 [API Reference](docs/API.md) - Full documentation
- 🔧 [Building from Source](docs/BUILDING.md) - Compile yourself

## Supported Platforms

| Platform | Status |
|----------|--------|
| Windows x64 | ✅ Pre-built |
| Linux x64 | 🔧 Build from source |
| macOS x64/arm64 | 🔧 Build from source |

## Building

```bash
git clone https://github.com/yourusername/UniversalBridge.git
cd UniversalBridge
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## License

MIT License - Free for personal and commercial use.

## Contributing

Contributions are welcome! Please read our contributing guidelines before submitting PRs.
