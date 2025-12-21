/**
 * =============================================================================
 * BRIDGE/INDEX.JS - CPPBridge v3.1 Hot-Reload Smart Loader
 * =============================================================================
 * 
 * Features:
 * - Shadow Copy: Copies DLL to temp file so original isn't locked
 * - Auto-Discovery: Reads registry.json and binds all functions
 * - Environment Detection: Works in Electron, React Native, Browser
 * - Hot Reload: Allows recompiling while app is running
 * 
 * Usage:
 *   const bridge = require('cppbridge');
 *   bridge.add(1, 2);  // Calls C++ function
 *   bridge.reload();   // Hot-reload after recompiling
 * 
 * =============================================================================
 */

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const os = require('os');

// =============================================================================
// CONFIGURATION
// =============================================================================

const CONFIG = {
    dllName: 'cppbridge',
    registryName: 'registry.json',
    shadowPrefix: 'cppbridge_shadow_',
};

let loadedLib = null;
let loadedFunctions = {};
let shadowPath = null;

// =============================================================================
// ENVIRONMENT DETECTION
// =============================================================================

function detectEnvironment() {
    if (typeof global !== 'undefined' && global.__fbBatchedBridge) {
        return 'react-native';
    }
    if (typeof window !== 'undefined' && typeof process === 'undefined') {
        return 'browser';
    }
    if (typeof window !== 'undefined' && typeof process !== 'undefined' && process.type === 'renderer') {
        return 'electron-renderer';
    }
    if (typeof process !== 'undefined' && process.versions && process.versions.node) {
        return process.versions.electron ? 'electron-main' : 'node';
    }
    return 'unknown';
}

// =============================================================================
// TYPE MAPPING (Registry types to Koffi types)
// =============================================================================

const TYPE_MAP = {
    'int': 'int',
    'float': 'float',
    'double': 'double',
    'bool': 'bool',
    'void': 'void',
    'str': 'str',
    'string': 'str',
    'const char*': 'str',
    'char*': 'str',
};

function mapType(registryType) {
    return TYPE_MAP[registryType] || registryType;
}

// =============================================================================
// DLL PATH FINDER
// =============================================================================

function findDllPath() {
    const ext = process.platform === 'win32' ? '.dll' :
        process.platform === 'darwin' ? '.dylib' : '.so';
    const dllName = CONFIG.dllName + ext;

    const searchPaths = [
        __dirname,
        path.join(__dirname, '..'),
        path.join(__dirname, '..', 'bridge'),
        process.cwd(),
        path.join(process.cwd(), 'bridge'),
    ];

    // Electron packaged app
    if (process.resourcesPath) {
        searchPaths.push(process.resourcesPath);
        searchPaths.push(path.join(process.resourcesPath, 'app'));
        searchPaths.push(path.join(process.resourcesPath, 'app', 'bridge'));
    }

    for (const dir of searchPaths) {
        const fullPath = path.join(dir, dllName);
        if (fs.existsSync(fullPath)) {
            return fullPath;
        }
    }

    throw new Error(
        `CPPBridge: DLL not found!\n` +
        `Run 'npm run build:bridge' to compile your C++ code.\n` +
        `Looking for: ${dllName}`
    );
}

// =============================================================================
// SHADOW COPY STRATEGY (Hot-Reload Support)
// =============================================================================
//
// Problem: On Windows, a loaded DLL is locked and cannot be overwritten.
// Solution: Copy the DLL to a temp file with a unique hash, load the copy.
// Benefit: The original DLL remains unlocked for recompilation.
//

function createShadowCopy(originalPath) {
    const tempDir = os.tmpdir();
    const hash = crypto.createHash('md5')
        .update(fs.readFileSync(originalPath))
        .update(Date.now().toString())
        .digest('hex')
        .substring(0, 8);

    const ext = path.extname(originalPath);
    const shadowName = `${CONFIG.shadowPrefix}${hash}${ext}`;
    const shadowFullPath = path.join(tempDir, shadowName);

    // Copy the DLL to temp location
    fs.copyFileSync(originalPath, shadowFullPath);

    console.log(`[CPPBridge] Shadow copy: ${shadowName}`);
    return shadowFullPath;
}

function cleanOldShadowCopies() {
    try {
        const tempDir = os.tmpdir();
        const files = fs.readdirSync(tempDir);

        for (const file of files) {
            if (file.startsWith(CONFIG.shadowPrefix)) {
                try {
                    fs.unlinkSync(path.join(tempDir, file));
                } catch (e) {
                    // File might be in use, ignore
                }
            }
        }
    } catch (e) {
        // Ignore cleanup errors
    }
}

// =============================================================================
// REGISTRY LOADER
// =============================================================================

function loadRegistry() {
    const registryPaths = [
        path.join(__dirname, CONFIG.registryName),
        path.join(__dirname, '..', 'bridge', CONFIG.registryName),
        path.join(__dirname, 'bridge_registry.json'),
        path.join(process.cwd(), 'bridge', CONFIG.registryName),
    ];

    for (const regPath of registryPaths) {
        if (fs.existsSync(regPath)) {
            console.log(`[CPPBridge] Registry: ${path.basename(regPath)}`);
            return JSON.parse(fs.readFileSync(regPath, 'utf8'));
        }
    }

    console.warn('[CPPBridge] No registry found, using empty');
    return { version: '3.1.0', functions: {} };
}

// =============================================================================
// KOFFI LOADER (For Electron/Node.js)
// =============================================================================

function loadWithKoffi() {
    let koffi;
    try {
        koffi = require('koffi');
    } catch (e) {
        throw new Error(
            `CPPBridge: Koffi not installed!\n` +
            `Run: npm install koffi`
        );
    }

    // Clean old shadow copies before loading new one
    cleanOldShadowCopies();

    const originalPath = findDllPath();
    const registry = loadRegistry();

    // Create shadow copy for hot-reload support
    shadowPath = createShadowCopy(originalPath);

    console.log(`[CPPBridge] Loading: ${path.basename(originalPath)}`);

    loadedLib = koffi.load(shadowPath);
    loadedFunctions = {};

    // Bind all functions from registry
    for (const [name, info] of Object.entries(registry.functions)) {
        try {
            const returnType = mapType(info.returnType);
            const paramTypes = info.params.map(mapType);

            loadedFunctions[name] = loadedLib.func(name, returnType, paramTypes);

        } catch (e) {
            console.warn(`[CPPBridge] Warning: Could not bind ${name}: ${e.message}`);
        }
    }

    console.log(`[CPPBridge] Loaded ${Object.keys(loadedFunctions).length} functions`);

    return loadedFunctions;
}

// =============================================================================
// HOT RELOAD
// =============================================================================

function reload() {
    console.log('[CPPBridge] Reloading...');

    // Unload current library if koffi supports it
    if (loadedLib && typeof loadedLib.close === 'function') {
        try {
            loadedLib.close();
        } catch (e) {
            // Ignore close errors
        }
    }

    // Load fresh copy
    const newFunctions = loadWithKoffi();

    // Update the exported bridge object
    for (const key of Object.keys(bridge)) {
        if (typeof bridge[key] === 'function' && key !== 'reload' && key !== '_getInfo') {
            delete bridge[key];
        }
    }

    Object.assign(bridge, newFunctions);

    console.log('[CPPBridge] Reload complete!');
    return bridge;
}

// =============================================================================
// JSI LOADER (For React Native)
// =============================================================================

function loadWithJSI() {
    const registry = loadRegistry();
    const funcs = {};

    for (const name of Object.keys(registry.functions)) {
        if (typeof global[name] === 'function') {
            funcs[name] = global[name];
        }
    }

    return funcs;
}

// =============================================================================
// BRIDGE INFO
// =============================================================================

function getInfo() {
    return {
        version: '3.1.0',
        environment: detectEnvironment(),
        shadowPath: shadowPath,
        loadedFunctions: Object.keys(loadedFunctions),
    };
}

// =============================================================================
// MAIN EXPORT
// =============================================================================

const environment = detectEnvironment();
let bridge = {};

try {
    switch (environment) {
        case 'electron-main':
        case 'electron-renderer':
        case 'node':
            bridge = loadWithKoffi();
            break;

        case 'react-native':
            bridge = loadWithJSI();
            break;

        case 'browser':
            console.log('[CPPBridge] Browser mode - use WASM build');
            break;

        default:
            console.warn(`[CPPBridge] Unknown environment: ${environment}`);
            bridge = loadWithKoffi();
    }
} catch (e) {
    console.error('[CPPBridge] Load error:', e.message);
    bridge = {};
}

// Add utility functions
bridge.reload = reload;
bridge._getInfo = getInfo;
bridge._version = '3.1.0';
bridge._environment = environment;

module.exports = bridge;
