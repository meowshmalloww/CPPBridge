/**
 * =============================================================================
 * AUTO-BRIDGE.JS - Zero-Config JavaScript Bridge
 * =============================================================================
 * 
 * Just require this file and call your C++ functions!
 * 
 * USAGE:
 *   const cpp = require('./auto-bridge');
 *   
 *   console.log(cpp.greet("World"));     // Calls C++ greet()
 *   console.log(cpp.add(5, 3));          // Calls C++ add()
 *   console.log(cpp.bridgeTest());       // "✅ CPPBridge is connected!"
 * 
 * =============================================================================
 */

const koffi = require('koffi');
const path = require('path');
const fs = require('fs');

// =============================================================================
// FIND THE DLL
// =============================================================================

function findDLL() {
    const names = ['backend.dll', 'cppbridge.dll', 'UniversalBridge.dll'];
    const dirs = [
        __dirname,
        path.join(__dirname, '..'),
        path.join(__dirname, 'build'),
        path.join(__dirname, '..', 'build'),
    ];

    for (const dir of dirs) {
        for (const name of names) {
            const p = path.join(dir, name);
            if (fs.existsSync(p)) {
                return p;
            }
        }
    }

    throw new Error('No DLL found! Run build.bat first.');
}

// =============================================================================
// AUTO-DISCOVER FUNCTIONS
// =============================================================================

let lib = null;
const functions = {};

// Common function signatures to try
const SIGNATURES = {
    // Functions that return strings
    'bridgeTest': ['const char*', []],
    'bridgeVersion': ['const char*', []],
    'bridgeEcho': ['const char*', ['const char*']],
    'greet': ['const char*', ['const char*']],
    'backendMessage': ['const char*', []],
    'backendAction': ['const char*', ['const char*']],
    'getVersion': ['const char*', []],

    // Functions that return numbers
    'add': ['int', ['int', 'int']],
    'multiply': ['double', ['double', 'double']],
    'calculate': ['double', ['double', 'double']],
};

function loadLibrary() {
    const dllPath = findDLL();
    console.log(`[CPPBridge] Loading: ${dllPath}`);

    lib = koffi.load(dllPath);

    // Try to bind known functions
    for (const [name, [ret, params]] of Object.entries(SIGNATURES)) {
        try {
            functions[name] = lib.func(name, ret, params);
            console.log(`[CPPBridge] ✓ ${name}`);
        } catch (e) {
            // Function doesn't exist, skip it
        }
    }

    console.log(`[CPPBridge] Loaded ${Object.keys(functions).length} functions`);
}

// =============================================================================
// DYNAMIC FUNCTION LOADER
// =============================================================================

// Call any function by name (auto-detect signature)
function call(name, ...args) {
    if (functions[name]) {
        return functions[name](...args);
    }

    // Try to load it dynamically
    const paramTypes = args.map(arg => {
        if (typeof arg === 'string') return 'const char*';
        if (typeof arg === 'number') return Number.isInteger(arg) ? 'int' : 'double';
        return 'void*';
    });

    // Guess return type based on function name
    let returnType = 'const char*'; // Default to string
    if (name.startsWith('get') || name.startsWith('is') || name.startsWith('has')) {
        returnType = 'const char*';
    } else if (name.startsWith('add') || name.startsWith('count') || name.startsWith('calc')) {
        returnType = 'double';
    }

    try {
        const fn = lib.func(name, returnType, paramTypes);
        functions[name] = fn;
        return fn(...args);
    } catch (e) {
        throw new Error(`Function '${name}' not found in DLL`);
    }
}

// Register a custom function signature
function register(name, returnType, paramTypes) {
    if (!lib) loadLibrary();
    functions[name] = lib.func(name, returnType, paramTypes);
    return functions[name];
}

// =============================================================================
// LOAD ON REQUIRE
// =============================================================================

try {
    loadLibrary();
} catch (e) {
    console.error('[CPPBridge] Warning:', e.message);
}

// =============================================================================
// EXPORTS - Use like: cpp.greet("World")
// =============================================================================

module.exports = new Proxy(functions, {
    get(target, prop) {
        if (prop === 'call') return call;
        if (prop === 'register') return register;
        if (prop === 'reload') return loadLibrary;
        if (target[prop]) return target[prop];

        // Try to load function on-demand
        return (...args) => call(prop, ...args);
    }
});
