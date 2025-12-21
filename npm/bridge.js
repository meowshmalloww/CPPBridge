/**
 * =============================================================================
 * BRIDGE.JS - Universal JavaScript Bridge for C++ Backends
 * =============================================================================
 * 
 * Automatically loads C++ functions from a DLL/SO/DYLIB using koffi.
 * 
 * INSTALLATION:
 *   npm install koffi
 * 
 * USAGE:
 *   1. Define your functions in the FUNCTIONS config below
 *   2. Import and use: const { add, greet } = require('./bridge');
 * 
 * Works with: Electron, Node.js, NW.js
 * =============================================================================
 */

const koffi = require('koffi');
const path = require('path');
const fs = require('fs');

// =============================================================================
// CONFIGURATION - Define your C++ functions here
// =============================================================================
// Format: 'functionName': ['returnType', ['param1Type', 'param2Type', ...]]
//
// Supported types:
//   - 'void', 'int', 'float', 'double', 'bool'
//   - 'string' (mapped to 'const char*')
//   - 'pointer' (mapped to 'void*')
//   - 'buffer' (for binary data)
// =============================================================================

const FUNCTIONS = {
    // ===== Example Functions (matches backend_export.h examples) =====
    'add': ['int', ['int', 'int']],
    'greet': ['string', ['string']],
    'backendMessage': ['string', []],
    'getVersion': ['string', []],

    // ===== Your Custom Functions =====
    // Add your functions here:
    // 'myFunction':  ['string', ['int', 'string']],
    // 'calculate':   ['double', ['double', 'double']],
};

// =============================================================================
// LIBRARY PATH - Auto-detect based on platform
// =============================================================================

function getLibraryPath() {
    const platform = process.platform;
    const arch = process.arch;

    // Default library name
    let libName;
    if (platform === 'win32') {
        libName = 'UniversalBridge.dll';
    } else if (platform === 'darwin') {
        libName = 'libUniversalBridge.dylib';
    } else {
        libName = 'libUniversalBridge.so';
    }

    // Search paths (in order of priority)
    const searchPaths = [
        // Same directory as this script
        path.join(__dirname, libName),
        // Prebuilds directory (for npm packages)
        path.join(__dirname, 'prebuilds', `${platform}-${arch}`, libName),
        // Build directory
        path.join(__dirname, '..', 'build', libName),
        path.join(__dirname, '..', 'build', 'Release', libName),
        path.join(__dirname, '..', 'build', 'Debug', libName),
        // System paths
        libName, // Let the OS find it
    ];

    for (const p of searchPaths) {
        if (fs.existsSync(p)) {
            console.log(`[Bridge] Found library: ${p}`);
            return p;
        }
    }

    // Fallback - let koffi try to find it
    console.warn(`[Bridge] Library not found in search paths, trying: ${libName}`);
    return libName;
}

// =============================================================================
// TYPE MAPPING - Convert simple types to koffi types
// =============================================================================

function mapType(type) {
    const typeMap = {
        'void': 'void',
        'int': 'int',
        'float': 'float',
        'double': 'double',
        'bool': 'bool',
        'string': 'const char*',
        'pointer': 'void*',
        'buffer': 'uint8_t*',
        'uint': 'uint32_t',
        'int64': 'int64_t',
        'uint64': 'uint64_t',
    };
    return typeMap[type] || type;
}

// =============================================================================
// LOAD LIBRARY AND BIND FUNCTIONS
// =============================================================================

let lib = null;
const exports = {};

function loadLibrary() {
    if (lib) return; // Already loaded

    try {
        const libPath = getLibraryPath();
        lib = koffi.load(libPath);
        console.log(`[Bridge] Library loaded successfully`);

        // Bind all functions from config
        for (const [name, [returnType, paramTypes]] of Object.entries(FUNCTIONS)) {
            try {
                const mappedReturn = mapType(returnType);
                const mappedParams = paramTypes.map(mapType);

                exports[name] = lib.func(name, mappedReturn, mappedParams);
                console.log(`[Bridge] Bound: ${name}(${paramTypes.join(', ')}) -> ${returnType}`);
            } catch (err) {
                console.error(`[Bridge] Failed to bind ${name}: ${err.message}`);
                // Create a stub that throws an error
                exports[name] = () => {
                    throw new Error(`Function '${name}' failed to load: ${err.message}`);
                };
            }
        }
    } catch (err) {
        console.error(`[Bridge] Failed to load library: ${err.message}`);
        throw err;
    }
}

// =============================================================================
// AUTO-LOAD ON REQUIRE
// =============================================================================

try {
    loadLibrary();
} catch (err) {
    console.error('[Bridge] Library load failed:', err.message);
}

// =============================================================================
// EXPORTS
// =============================================================================

module.exports = {
    ...exports,

    // Utility functions
    reload: () => {
        lib = null;
        loadLibrary();
    },

    isLoaded: () => lib !== null,

    // For dynamic function loading
    loadFunction: (name, returnType, paramTypes) => {
        if (!lib) throw new Error('Library not loaded');
        const fn = lib.func(name, mapType(returnType), paramTypes.map(mapType));
        exports[name] = fn;
        return fn;
    },
};
