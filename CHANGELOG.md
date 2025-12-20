# Changelog

All notable changes to UniversalBridge will be documented in this file.

## [1.0.0] - 2025-12-20

### 🎉 Initial Release

**Core Features:**
- Thread pool with priority queues
- Event bus for push notifications
- Async task system with futures

**Security:**
- AES-256-GCM encryption with hardware acceleration
- SHA-256 and PBKDF2 hashing
- Path sandboxing and input validation

**Networking:**
- HTTP client with connection pooling and retry
- WebSocket client with auto-reconnect
- Binary message support

**Storage:**
- SQLite database with ORM
- Key-value store (persistent)
- Memory-mapped files with auto-chunking

**System:**
- Process management (list, spawn, kill)
- Registry access (read, write, enumerate)
- Clipboard operations
- Environment variables

**Logging:**
- Async logging with rotation
- JSON structured output
- Multiple severity levels

**Compression:**
- Huffman coding
- ZLIB-compatible format

### Platforms
- Windows x64 (pre-built)
- Linux x64 (build from source)
- macOS x64/arm64 (build from source)
