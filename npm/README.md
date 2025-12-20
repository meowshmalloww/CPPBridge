# UniversalBridge

Native C++ backend for React Native, Electron, and Node.js.

## Installation

```bash
npm install universalbridge
```

## Usage

```javascript
import { http, database, keyvalue, system, logging } from 'universalbridge';

// HTTP requests with retry
const response = http.get('https://api.example.com/users');
console.log(response.status, response.body);

// Database
const db = database.open('myapp.db');
db.exec('CREATE TABLE users (id INTEGER, name TEXT)');
const users = db.query('SELECT * FROM users');
db.close();

// Key-Value Store (perfect for app settings)
const store = keyvalue.open('settings.db');
store.set('theme', 'dark');
store.set('user_id', '12345');
console.log(store.get('theme'));  // 'dark'
store.close();

// System utilities
console.log(system.process.cpuCount());  // 8
console.log(system.env.get('HOME'));     // '/Users/you'

// Logging
logging.init('./logs', 'myapp');
logging.info('Application started');
logging.error('Something went wrong', 'AuthModule');
```

## Supported Platforms

- Windows x64
- Linux x64
- macOS x64 (Intel)
- macOS arm64 (Apple Silicon)

## License

MIT
