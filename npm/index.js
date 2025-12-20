/**
 * UniversalBridge - Native C++ Backend for JavaScript
 * 
 * Usage:
 *   import { http, crypto, database, system } from 'universalbridge';
 *   
 *   // HTTP requests
 *   const response = http.get('https://api.example.com');
 *   
 *   // Encryption
 *   const encrypted = crypto.aesEncrypt(data, key);
 *   
 *   // Database
 *   database.open('myapp.db');
 *   database.query('SELECT * FROM users');
 */

const ffi = require('ffi-napi');
const path = require('path');
const os = require('os');

// Detect platform and load appropriate binary
function getLibraryPath() {
    const platform = os.platform();
    const arch = os.arch();

    const platformMap = {
        'win32': 'UniversalBridge.dll',
        'linux': 'libuniversalbridge.so',
        'darwin': 'libuniversalbridge.dylib'
    };

    const libName = platformMap[platform];
    if (!libName) {
        throw new Error(`Unsupported platform: ${platform}`);
    }

    return path.join(__dirname, 'prebuilds', `${platform}-${arch}`, libName);
}

// Load native library
let lib;
try {
    lib = ffi.Library(getLibraryPath(), {
        // HTTP Client
        'hub_http_adv_get': ['int', ['string', 'int']],
        'hub_http_adv_post': ['int', ['string', 'string', 'string', 'int']],
        'hub_http_adv_status': ['int', ['int']],
        'hub_http_adv_body': ['string', ['int']],
        'hub_http_adv_release': ['void', ['int']],

        // Crypto
        'hub_crypto_sha256': ['int', ['string', 'int', 'pointer', 'int']],
        'hub_crypto_aes_encrypt': ['int', ['pointer', 'int', 'pointer', 'pointer', 'int']],
        'hub_crypto_aes_decrypt': ['int', ['pointer', 'int', 'pointer', 'pointer', 'int']],

        // Database
        'hub_db_open': ['int', ['string']],
        'hub_db_close': ['void', ['int']],
        'hub_db_exec': ['int', ['int', 'string']],
        'hub_db_query': ['string', ['int', 'string']],

        // Key-Value Store
        'hub_kv_open': ['int', ['string']],
        'hub_kv_set': ['int', ['int', 'string', 'string']],
        'hub_kv_get': ['string', ['int', 'string']],
        'hub_kv_delete': ['int', ['int', 'string']],
        'hub_kv_close': ['void', ['int']],

        // Logging
        'hub_log_adv_init': ['void', ['string', 'string', 'int', 'int']],
        'hub_log_adv': ['void', ['int', 'string', 'string']],
        'hub_log_adv_shutdown': ['void', []],

        // Process
        'hub_proc_run_capture': ['int', ['string', 'pointer', 'int']],
        'hub_proc_spawn': ['uint', ['string', 'string', 'int']],
        'hub_proc_kill': ['int', ['uint']],

        // System
        'hub_env_get': ['string', ['string']],
        'hub_env_set': ['int', ['string', 'string']],
        'hub_process_id': ['uint', []],
        'hub_cpu_count': ['uint', []]
    });
} catch (e) {
    console.error('Failed to load UniversalBridge native library:', e.message);
    console.error('Make sure the prebuilt binaries are installed for your platform.');
    throw e;
}

// ============================================================================
// HTTP Module
// ============================================================================
const http = {
    get(url, retries = 0) {
        const handle = lib.hub_http_adv_get(url, retries);
        const status = lib.hub_http_adv_status(handle);
        const body = lib.hub_http_adv_body(handle);
        lib.hub_http_adv_release(handle);
        return { status, body };
    },

    post(url, body, contentType = 'application/json', retries = 0) {
        const handle = lib.hub_http_adv_post(url, body, contentType, retries);
        const status = lib.hub_http_adv_status(handle);
        const responseBody = lib.hub_http_adv_body(handle);
        lib.hub_http_adv_release(handle);
        return { status, body: responseBody };
    }
};

// ============================================================================
// Database Module
// ============================================================================
class Database {
    constructor() {
        this.handle = -1;
    }

    open(path) {
        this.handle = lib.hub_db_open(path);
        return this.handle >= 0;
    }

    exec(sql) {
        return lib.hub_db_exec(this.handle, sql) === 0;
    }

    query(sql) {
        const result = lib.hub_db_query(this.handle, sql);
        try {
            return JSON.parse(result);
        } catch {
            return result;
        }
    }

    close() {
        lib.hub_db_close(this.handle);
        this.handle = -1;
    }
}

const database = {
    Database,
    open(path) {
        const db = new Database();
        db.open(path);
        return db;
    }
};

// ============================================================================
// Key-Value Store Module
// ============================================================================
class KVStore {
    constructor() {
        this.handle = -1;
    }

    open(path) {
        this.handle = lib.hub_kv_open(path);
        return this.handle >= 0;
    }

    set(key, value) {
        return lib.hub_kv_set(this.handle, key, String(value)) === 1;
    }

    get(key) {
        return lib.hub_kv_get(this.handle, key);
    }

    delete(key) {
        return lib.hub_kv_delete(this.handle, key) === 1;
    }

    close() {
        lib.hub_kv_close(this.handle);
        this.handle = -1;
    }
}

const keyvalue = {
    KVStore,
    open(path) {
        const store = new KVStore();
        store.open(path);
        return store;
    }
};

// ============================================================================
// System Module  
// ============================================================================
const system = {
    env: {
        get: (name) => lib.hub_env_get(name),
        set: (name, value) => lib.hub_env_set(name, value) === 1
    },

    process: {
        id: () => lib.hub_process_id(),
        cpuCount: () => lib.hub_cpu_count(),
        spawn: (command, workDir = '', hidden = false) =>
            lib.hub_proc_spawn(command, workDir, hidden ? 1 : 0),
        kill: (pid) => lib.hub_proc_kill(pid) === 1
    }
};

// ============================================================================
// Logging Module
// ============================================================================
const LogLevel = { TRACE: 0, DEBUG: 1, INFO: 2, WARN: 3, ERROR: 4, FATAL: 5 };

const logging = {
    LogLevel,

    init(logDir, prefix = 'app', maxSizeMb = 10, maxFiles = 5) {
        lib.hub_log_adv_init(logDir, prefix, maxSizeMb, maxFiles);
    },

    log(level, message, source = '') {
        lib.hub_log_adv(level, message, source);
    },

    info: (msg, src) => lib.hub_log_adv(LogLevel.INFO, msg, src || ''),
    warn: (msg, src) => lib.hub_log_adv(LogLevel.WARN, msg, src || ''),
    error: (msg, src) => lib.hub_log_adv(LogLevel.ERROR, msg, src || ''),

    shutdown() {
        lib.hub_log_adv_shutdown();
    }
};

// ============================================================================
// Exports
// ============================================================================
module.exports = {
    http,
    database,
    keyvalue,
    system,
    logging,

    // Direct access to native library for advanced users
    _native: lib
};
