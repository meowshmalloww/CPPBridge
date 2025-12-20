# 📖 API Reference

Complete API documentation for UniversalBridge.

---

## Modules Overview

| Module | Description |
|--------|-------------|
| `http` | HTTP client with retry and connection pooling |
| `database` | SQLite database wrapper |
| `keyvalue` | Persistent key-value storage |
| `system` | Environment, process management |
| `logging` | Async file logging with rotation |

---

## http

HTTP client with automatic retry and connection pooling.

### `http.get(url, retries?)`

Make a GET request.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| url | string | required | Full URL to request |
| retries | number | 0 | Number of retry attempts |

**Returns:** `{ status: number, body: string }`

```javascript
const response = http.get('https://api.github.com/users/octocat');
console.log(response.status);  // 200
console.log(JSON.parse(response.body));  // User object
```

### `http.post(url, body, contentType?, retries?)`

Make a POST request.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| url | string | required | Full URL |
| body | string | required | Request body |
| contentType | string | 'application/json' | Content-Type header |
| retries | number | 0 | Retry attempts |

```javascript
const response = http.post(
  'https://api.example.com/users',
  JSON.stringify({ name: 'John' }),
  'application/json',
  3  // Retry up to 3 times
);
```

---

## database

SQLite database with query builder.

### `database.open(path)`

Open or create a database file.

```javascript
const db = database.open('myapp.db');
```

### `db.exec(sql)`

Execute SQL statement (no return value).

```javascript
db.exec('CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)');
db.exec('INSERT INTO users (name) VALUES ("John")');
```

### `db.query(sql)`

Execute SQL and return results.

```javascript
const users = db.query('SELECT * FROM users WHERE id = 1');
// Returns: [{ id: 1, name: 'John' }]
```

### `db.close()`

Close the database connection.

---

## keyvalue

Persistent key-value store backed by SQLite.

### `keyvalue.open(path)`

Open or create a key-value store.

```javascript
const store = keyvalue.open('settings.db');
```

### `store.set(key, value)`

Store a value.

```javascript
store.set('theme', 'dark');
store.set('user_id', '12345');
store.set('preferences', JSON.stringify({ sound: true }));
```

### `store.get(key)`

Retrieve a value. Returns `null` if not found.

```javascript
const theme = store.get('theme');  // 'dark'
const missing = store.get('unknown');  // null
```

### `store.delete(key)`

Remove a key-value pair.

```javascript
store.delete('theme');
```

### `store.close()`

Close the store.

---

## system

System utilities and process management.

### `system.env.get(name)`

Get environment variable.

```javascript
const home = system.env.get('HOME');
const path = system.env.get('PATH');
```

### `system.env.set(name, value)`

Set environment variable (current process only).

```javascript
system.env.set('MY_VAR', 'hello');
```

### `system.process.id()`

Get current process ID.

```javascript
console.log(system.process.id());  // 12345
```

### `system.process.cpuCount()`

Get number of CPU cores.

```javascript
console.log(system.process.cpuCount());  // 8
```

### `system.process.spawn(command, workDir?, hidden?)`

Start a new process. Returns process ID.

```javascript
const pid = system.process.spawn('notepad.exe', '', false);
```

### `system.process.kill(pid)`

Terminate a process.

```javascript
system.process.kill(12345);
```

---

## logging

Async file logging with rotation.

### `logging.init(logDir, prefix?, maxSizeMb?, maxFiles?)`

Initialize the logger.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| logDir | string | required | Directory for log files |
| prefix | string | 'app' | Log file prefix |
| maxSizeMb | number | 10 | Max file size before rotation |
| maxFiles | number | 5 | Max number of log files kept |

```javascript
logging.init('./logs', 'myapp', 10, 5);
```

### `logging.info(message, source?)`

Log info message.

```javascript
logging.info('User logged in', 'AuthModule');
```

### `logging.warn(message, source?)`
### `logging.error(message, source?)`

Log warning or error.

```javascript
logging.warn('Low disk space');
logging.error('Database connection failed', 'DatabaseModule');
```

### `logging.shutdown()`

Flush and close the logger.

---

## Advanced: Direct Native Access

For advanced users, access the raw FFI bindings:

```javascript
const { _native } = require('universalbridge');

// Call any exported C function directly
const result = _native.hub_crypto_sha256(data, len, outBuffer, 32);
```

See the C++ header files for all available functions.
