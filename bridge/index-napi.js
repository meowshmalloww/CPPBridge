/**
 * =============================================================================
 * BRIDGE/INDEX-NAPI.JS - CPPBridge N-API Loader (Alternative to Koffi)
 * =============================================================================
 * 
 * N-API provides native Node.js bindings with better performance for
 * large data transfers but requires compiling a .node addon.
 * 
 * Use this if:
 *   - You need maximum performance
 *   - You're transferring large buffers (images, audio, etc.)
 *   - Koffi doesn't work in your environment
 * 
 * Use Koffi (default) if:
 *   - You want simplicity
 *   - You don't want to compile addons
 *   - Performance is good enough
 * 
 * =============================================================================
 */

const fs = require('fs');
const path = require('path');

// =============================================================================
// CONFIGURATION
// =============================================================================

const CONFIG = {
    addonName: 'cppbridge',
};

let loadedAddon = null;
let registryData = null;

// =============================================================================
// ADDON PATH FINDER
// =============================================================================

function findAddon() {
    const searchPaths = [
        __dirname,
        path.join(__dirname, 'build', 'Release'),
        path.join(__dirname, 'build', 'Debug'),
        path.join(__dirname, '..', 'build', 'Release'),
        path.join(process.cwd(), 'build', 'Release'),
    ];

    for (const dir of searchPaths) {
        const fullPath = path.join(dir, `${CONFIG.addonName}.node`);
        if (fs.existsSync(fullPath)) {
            return fullPath;
        }
    }

    throw new Error(
        'N-API addon not found!\n' +
        'Build with: npm run build:napi\n' +
        'Or use Koffi loader: require("./bridge")'
    );
}

// =============================================================================
// REGISTRY LOADER
// =============================================================================

function loadRegistry() {
    const paths = [
        path.join(__dirname, 'registry.json'),
        path.join(__dirname, '..', 'bridge', 'registry.json'),
    ];

    for (const p of paths) {
        if (fs.existsSync(p)) {
            return JSON.parse(fs.readFileSync(p, 'utf8'));
        }
    }

    return { version: '5.0.0', functions: {} };
}

// =============================================================================
// LOAD N-API ADDON
// =============================================================================

function load() {
    const addonPath = findAddon();
    loadedAddon = require(addonPath);
    registryData = loadRegistry();

    const bridge = {};

    // Bind functions from registry
    for (const [name, info] of Object.entries(registryData.functions)) {
        if (typeof loadedAddon[name] === 'function') {
            bridge[name] = loadedAddon[name];
        }
    }

    // Add built-in functions if available
    const builtins = ['_bridge_get_logs', '_store_get', '_store_set',
        '_store_get_int', '_store_set_int', '_store_dump'];

    for (const fn of builtins) {
        if (typeof loadedAddon[fn] === 'function') {
            bridge[fn] = loadedAddon[fn];
        }
    }

    // Create Store proxy
    if (loadedAddon._store_get) {
        bridge.Store = {
            get: (key) => loadedAddon._store_get(key),
            set: (key, val) => loadedAddon._store_set(key, String(val)),
            getInt: (key) => loadedAddon._store_get_int(key),
            setInt: (key, val) => loadedAddon._store_set_int(key, val),
            dump: () => JSON.parse(loadedAddon._store_dump() || '{}'),
        };
    }

    console.log(`[CPPBridge N-API] Loaded ${Object.keys(bridge).length} functions`);
    return bridge;
}

// =============================================================================
// RELOAD
// =============================================================================

function reload() {
    // Clear require cache
    const addonPath = findAddon();
    delete require.cache[require.resolve(addonPath)];

    const newBridge = load();
    Object.assign(bridge, newBridge);
    console.log('[CPPBridge N-API] Reloaded!');
    return bridge;
}

// =============================================================================
// EXPORT
// =============================================================================

let bridge = {};

try {
    bridge = load();
} catch (e) {
    console.error('[CPPBridge N-API]', e.message);
    console.log('Tip: Use Koffi loader instead: require("./bridge")');
}

bridge.reload = reload;
bridge._version = '5.0.0';
bridge._loader = 'napi';

module.exports = bridge;
