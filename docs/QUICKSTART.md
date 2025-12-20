# 🚀 Quickstart Guide

Get UniversalBridge running in 5 minutes!

---

## Step 1: Download

**Option A: Pre-built (Recommended)**
```bash
# Download from GitHub Releases
# Extract to your project folder
```

**Option B: npm**
```bash
npm install universalbridge
```

---

## Step 2: Basic Setup

### For Electron / Node.js

```javascript
// In your main process
const { http, keyvalue, system } = require('universalbridge');

// Test it works
console.log('CPU cores:', system.process.cpuCount());
```

### For React Native

```javascript
// Install native module support first
npm install react-native-universalbridge

// Then use normally
import { http, database } from 'universalbridge';
```

---

## Step 3: Your First App

Here's a complete example - a settings manager:

```javascript
const { keyvalue } = require('universalbridge');

// Open (creates if doesn't exist)
const settings = keyvalue.open('my-app-settings.db');

// Save settings
settings.set('username', 'john_doe');
settings.set('theme', 'dark');
settings.set('volume', '75');

// Read settings
console.log(settings.get('theme'));     // 'dark'
console.log(settings.get('username'));  // 'john_doe'

// Delete a setting
settings.delete('volume');

// Always close when done
settings.close();
```

---

## Common Tasks

### Make HTTP Requests

```javascript
const { http } = require('universalbridge');

// GET request
const users = http.get('https://api.example.com/users');
console.log(users.body);  // JSON string

// POST request
const result = http.post(
  'https://api.example.com/login',
  JSON.stringify({ email: 'user@example.com', password: '123' }),
  'application/json'
);
```

### Use a Database

```javascript
const { database } = require('universalbridge');

const db = database.open('myapp.db');

// Create table
db.exec(`
  CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY,
    name TEXT,
    email TEXT
  )
`);

// Insert data
db.exec(`INSERT INTO users (name, email) VALUES ('John', 'john@example.com')`);

// Query data
const users = db.query('SELECT * FROM users');
console.log(users);

db.close();
```

### Logging

```javascript
const { logging } = require('universalbridge');

// Initialize (logs go to ./logs folder)
logging.init('./logs', 'myapp');

// Log messages
logging.info('Application started');
logging.warn('Low memory warning');
logging.error('Failed to connect to server');

// Cleanup
logging.shutdown();
```

---

## Troubleshooting

### "Cannot find module 'universalbridge'"
```bash
npm install universalbridge
```

### "Failed to load native library"
Make sure the DLL/so/dylib exists in `node_modules/universalbridge/prebuilds/`

### "Function not found"
The C++ library may not be compiled with that feature. Check the API reference.

---

## Next Steps

- 📖 Read the [API Reference](./API.md) for all available functions
- 🔧 See [Building from Source](./BUILDING.md) to compile yourself
- 💬 Open an issue on GitHub for help
