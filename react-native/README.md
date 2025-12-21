# React Native CPPBridge

Native C++ backend for React Native iOS and Android apps.

## Installation

### 1. Copy to your project
```bash
cp -r react-native-cppbridge your-app/
cd your-app
npm install ./react-native-cppbridge
```

### 2. iOS - Link with CocoaPods
```bash
cd ios && pod install
```

### 3. Android - No extra steps
Android auto-links via React Native's autolinking.

## Usage

```typescript
import { http, database, keyvalue, crypto } from 'react-native-cppbridge';

// HTTP Request
const response = await http.get('https://api.example.com/users');
console.log(response.status); // 200

// Database
const db = await database.open('app.db');
await database.exec(db, 'CREATE TABLE users (id INTEGER, name TEXT)');
const users = await database.query(db, 'SELECT * FROM users');
await database.close(db);

// Key-Value Store
const store = await keyvalue.open('settings.db');
await keyvalue.set(store, 'theme', 'dark');
const theme = await keyvalue.get(store, 'theme'); // 'dark'
await keyvalue.close(store);

// Crypto
const hash = await crypto.sha256('password123');
console.log(hash); // hex string
```

## Platform Support

| Platform | Status |
|----------|--------|
| iOS 11+ | ✅ Supported |
| Android API 21+ | ✅ Supported |

## Build Requirements

- **iOS**: Xcode 12+, CocoaPods
- **Android**: Android Studio, NDK 21+
