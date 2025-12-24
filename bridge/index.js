/**
 * =============================================================================
 * BRIDGE/INDEX.JS - CPPBridge v6.1 Production Loader
 * =============================================================================
 * 
 * v6.1 Production Features:
 * - DLL Hot-Swap: Versioned DLLs to avoid Windows file locks
 * - Zero-Copy Buffers: Direct Uint8Array ↔ C++ memory
 * - Auto-Cleanup: FinalizationRegistry for memory safety
 * - TypeScript Types: Full autocomplete support
 * - Shadow Copy: Hot reload without restart
 * 
 * =============================================================================
 */

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const os = require('os');
const { Worker } = require('worker_threads');

// =============================================================================
// CONFIGURATION
// =============================================================================

const CONFIG = {
    dllName: 'cppbridge',
    shadowPrefix: 'cppbridge_',
    consolePollInterval: 100,
};

let loadedLib = null;
let koffi = null;
let shadowPath = null;
let registryData = null;
let currentDllPath = null;

// =============================================================================
// FEATURE 1: DLL HOT-SWAP (Versioned Loading)
// =============================================================================
// On Windows, DLLs are locked when loaded. This scans for versioned DLLs
// and loads the newest one, allowing build scripts to write new versions.

function findVersionedDll() {
    const ext = process.platform === 'win32' ? '.dll' :
        process.platform === 'darwin' ? '.dylib' : '.so';

    const searchDirs = [
        __dirname,
        path.join(__dirname, '..', 'build', 'Release'),
        path.join(__dirname, '..', 'build', 'Debug'),
        path.join(process.cwd(), 'build', 'Release'),
        path.join(process.cwd(), 'bridge'),
    ];

    let candidates = [];

    for (const dir of searchDirs) {
        if (!fs.existsSync(dir)) continue;

        try {
            const files = fs.readdirSync(dir);
            for (const file of files) {
                // Match: cppbridge.dll, cppbridge.1234567890.dll, UniversalBridge.dll
                if (file.endsWith(ext) &&
                    (file.startsWith(CONFIG.dllName) || file.startsWith('UniversalBridge'))) {
                    const fullPath = path.join(dir, file);
                    const stat = fs.statSync(fullPath);
                    candidates.push({
                        path: fullPath,
                        mtime: stat.mtimeMs,
                        name: file
                    });
                }
            }
        } catch (e) { }
    }

    if (candidates.length === 0) {
        throw new Error('No DLL found! Run: npm run build');
    }

    // Sort by modification time, newest first
    candidates.sort((a, b) => b.mtime - a.mtime);

    return candidates[0].path;
}

// =============================================================================
// FINALIZATION REGISTRY (Auto-Cleanup)
// =============================================================================

const _cleanupRegistry = new FinalizationRegistry(({ className, handle, deleteFn }) => {
    try {
        if (deleteFn) deleteFn(handle);
    } catch (e) { }
});

const _classDeleteFns = new Map();

function createManagedObject(className, handle) {
    const wrapper = { handle, className };
    const deleteFn = _classDeleteFns.get(className);
    _cleanupRegistry.register(wrapper, { className, handle, deleteFn });
    return wrapper;
}

function registerClassCleanup(className, deleteFn) {
    _classDeleteFns.set(className, deleteFn);
}

// =============================================================================
// PUSH CALLBACKS
// =============================================================================

const _jsCallbacks = new Map();

function registerCallback(name, fn) {
    if (typeof fn !== 'function') {
        throw new Error('Callback must be a function');
    }
    _jsCallbacks.set(name, fn);
}

// =============================================================================
// FEATURE 2: ZERO-COPY BUFFERS
// =============================================================================
// Bind functions that accept Uint8Array for direct memory access

function bindBufferFunction(lib, name, info) {
    // Check if any param is a buffer type
    const hasBuffer = info.params.some(p => p === 'buffer' || p === 'uint8*');

    if (!hasBuffer) return null;

    // Create Koffi signature with pointer types
    const returnType = TYPE_MAP[info.returnType] || info.returnType;
    const paramTypes = info.params.map(t => {
        if (t === 'buffer' || t === 'uint8*') return 'uint8*';
        return TYPE_MAP[t] || t;
    });

    const fn = lib.func(name, returnType, paramTypes);

    // Return wrapper that handles Uint8Array conversion
    return (...args) => {
        const convertedArgs = args.map((arg, i) => {
            if (arg instanceof Uint8Array) {
                return arg; // Koffi handles Uint8Array directly
            }
            return arg;
        });
        return fn(...convertedArgs);
    };
}

// =============================================================================
// TYPE MAPPING
// =============================================================================

const TYPE_MAP = {
    'int': 'int', 'float': 'float', 'double': 'double',
    'bool': 'bool', 'void': 'void', 'str': 'str', 'string': 'str',
    'uint64': 'uint64', 'int64': 'int64',
    'buffer': 'uint8*', 'uint8*': 'uint8*',
};

// =============================================================================
// SHADOW COPY (Hot Reload Support)
// =============================================================================

function createShadowCopy(originalPath) {
    const tempDir = os.tmpdir();
    const hash = crypto.randomBytes(4).toString('hex');
    const ext = path.extname(originalPath);
    const shadowFullPath = path.join(tempDir, `${CONFIG.shadowPrefix}${hash}${ext}`);
    fs.copyFileSync(originalPath, shadowFullPath);
    return shadowFullPath;
}

function cleanShadowCopies() {
    try {
        const tempDir = os.tmpdir();
        for (const file of fs.readdirSync(tempDir)) {
            if (file.startsWith(CONFIG.shadowPrefix)) {
                try { fs.unlinkSync(path.join(tempDir, file)); } catch (e) { }
            }
        }
    } catch (e) { }
}

// =============================================================================
// REGISTRY LOADER
// =============================================================================

function loadRegistry() {
    const paths = [
        path.join(__dirname, 'registry.json'),
        path.join(__dirname, '..', 'bridge', 'registry.json'),
        path.join(process.cwd(), 'bridge', 'registry.json'),
    ];

    for (const p of paths) {
        if (fs.existsSync(p)) {
            return JSON.parse(fs.readFileSync(p, 'utf8'));
        }
    }

    return { version: '6.1.0', functions: {} };
}

// =============================================================================
// ASYNC WORKER
// =============================================================================

function runInWorker(dllPath, funcName, returnType, paramTypes, args) {
    return new Promise((resolve, reject) => {
        const workerCode = `
            const { parentPort, workerData } = require('worker_threads');
            const koffi = require('koffi');
            try {
                const lib = koffi.load(workerData.dllPath);
                const fn = lib.func(workerData.funcName, workerData.returnType, workerData.paramTypes);
                const result = fn(...workerData.args);
                parentPort.postMessage({ success: true, result });
            } catch (error) {
                parentPort.postMessage({ success: false, error: error.message });
            }
        `;

        const worker = new Worker(workerCode, {
            eval: true,
            workerData: { dllPath, funcName, returnType, paramTypes, args }
        });

        worker.on('message', (msg) => {
            msg.success ? resolve(msg.result) : reject(new Error(msg.error));
            worker.terminate();
        });

        worker.on('error', (err) => {
            reject(err);
            worker.terminate();
        });
    });
}

// =============================================================================
// MAGIC STORE
// =============================================================================

function createStoreProxy(lib) {
    try {
        const _store_get = lib.func('_store_get', 'str', ['str']);
        const _store_set = lib.func('_store_set', 'void', ['str', 'str']);
        const _store_get_int = lib.func('_store_get_int', 'int', ['str']);
        const _store_set_int = lib.func('_store_set_int', 'void', ['str', 'int']);
        const _store_dump = lib.func('_store_dump', 'str', []);

        return {
            get: (key) => _store_get(key),
            set: (key, value) => _store_set(key, String(value)),
            getInt: (key) => _store_get_int(key),
            setInt: (key, value) => _store_set_int(key, value),
            dump: () => JSON.parse(_store_dump() || '{}'),
        };
    } catch (e) {
        return { get: () => null, set: () => { }, dump: () => ({}) };
    }
}

// =============================================================================
// CLASS FACTORY
// =============================================================================

function createClassFactory(lib, className) {
    try {
        const newFn = lib.func(`${className}_new`, 'int', []);
        const deleteFn = lib.func(`${className}_delete`, 'void', ['int']);

        registerClassCleanup(className, deleteFn);

        return {
            create: () => {
                const handle = newFn();
                return createManagedObject(className, handle);
            },
            delete: (wrapper) => {
                if (wrapper && wrapper.handle) {
                    deleteFn(wrapper.handle);
                    wrapper.handle = null;
                }
            }
        };
    } catch (e) {
        return null;
    }
}

// =============================================================================
// LOAD BRIDGE
// =============================================================================

function load() {
    try {
        koffi = require('koffi');
    } catch (e) {
        throw new Error('Koffi not installed! Run: npm install koffi');
    }

    cleanShadowCopies();

    // Use versioned DLL finder for hot-swap support
    const originalPath = findVersionedDll();
    currentDllPath = originalPath;
    registryData = loadRegistry();

    // Create shadow copy to avoid file locks
    shadowPath = createShadowCopy(originalPath);
    loadedLib = koffi.load(shadowPath);

    const bridge = {};
    let syncCount = 0, asyncCount = 0, bufferCount = 0;

    // Bind user functions
    for (const [name, info] of Object.entries(registryData.functions)) {
        try {
            const returnType = TYPE_MAP[info.returnType] || info.returnType;
            const paramTypes = info.params.map(t => TYPE_MAP[t] || t);

            // Check for buffer function first
            const bufferFn = bindBufferFunction(loadedLib, name, info);
            if (bufferFn) {
                bridge[name] = bufferFn;
                bufferCount++;
                continue;
            }

            if (info.async) {
                bridge[name] = (...args) => runInWorker(shadowPath, name, returnType, paramTypes, args);
                asyncCount++;
            } else {
                bridge[name] = loadedLib.func(name, returnType, paramTypes);
                syncCount++;
            }
        } catch (e) {
            console.warn(`[CPPBridge] Could not bind: ${name}`);
        }
    }

    // Bind Store
    bridge.Store = createStoreProxy(loadedLib);

    // Enterprise features
    bridge.registerCallback = registerCallback;
    bridge.createClass = (className) => createClassFactory(loadedLib, className);

    console.log(`[CPPBridge] Loaded: ${syncCount} sync, ${asyncCount} async, ${bufferCount} buffer`);
    console.log(`[CPPBridge] DLL: ${path.basename(currentDllPath)}`);

    return bridge;
}

// =============================================================================
// HOT RELOAD
// =============================================================================

function reload() {
    if (loadedLib && loadedLib.close) {
        try { loadedLib.close(); } catch (e) { }
    }

    const newBridge = load();

    const preserve = ['reload', '_version', '_getInfo', 'Store', 'registerCallback', 'createClass'];
    for (const key of Object.keys(bridge)) {
        if (!preserve.includes(key)) {
            delete bridge[key];
        }
    }
    Object.assign(bridge, newBridge);

    console.log('[CPPBridge] Hot reload complete!');
    return bridge;
}

// =============================================================================
// INFO
// =============================================================================

function getInfo() {
    return {
        version: '6.1.0',
        dllPath: currentDllPath,
        shadowPath: shadowPath,
        functions: registryData ? Object.keys(registryData.functions) : [],
        features: ['dll-hot-swap', 'zero-copy-buffers', 'auto-cleanup', 'typescript-types'],
    };
}

// =============================================================================
// EXPORT
// =============================================================================

let bridge = {};

try {
    bridge = load();
} catch (e) {
    console.error('[CPPBridge]', e.message);
}

bridge.reload = reload;
bridge._getInfo = getInfo;
bridge._version = '6.1.0';

module.exports = bridge;
