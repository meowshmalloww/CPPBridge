# UniversalBridge Documentation

Welcome to UniversalBridge - a native C++ backend library for JavaScript applications.

## 📚 Table of Contents

| Guide | Who It's For |
|-------|--------------|
| [Quickstart](./QUICKSTART.md) | Beginners - Get running in 5 minutes |
| [API Reference](./API.md) | Developers - Full API documentation |
| [Building from Source](./BUILDING.md) | Contributors - Compile yourself |

## What is UniversalBridge?

UniversalBridge is a **native C++ library** that provides:
- 🔒 **Security** - AES-256 encryption, SHA-256 hashing
- 🌐 **Networking** - HTTP client with retry, WebSocket
- 💾 **Storage** - SQLite database, key-value store
- 🖥️ **System** - Process management, registry, clipboard

## Supported Platforms

| Platform | Architecture | Status |
|----------|--------------|--------|
| Windows | x64 | ✅ Ready |
| Linux | x64 | 🔧 Build from source |
| macOS | x64/arm64 | 🔧 Build from source |

## Quick Example

```javascript
import { http, database, keyvalue } from 'universalbridge';

// Make HTTP request
const response = http.get('https://api.github.com');
console.log(response.status);  // 200

// Store settings
const store = keyvalue.open('settings.db');
store.set('theme', 'dark');
console.log(store.get('theme'));  // 'dark'
```

## License

MIT License - Free for personal and commercial use.
