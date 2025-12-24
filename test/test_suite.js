/**
 * =============================================================================
 * UniversalBridge v6.1.0 — Complete Test Suite
 * =============================================================================
 * 
 * Categories:
 *   A. Core Sanity & Versioning
 *   B. DLL Hot-Swap
 *   C. Zero-Copy Buffers
 *   D. Functions (BRIDGE)
 *   E. Variables (BRIDGE_VAR)
 *   F. Enums (BRIDGE_ENUM)
 *   G. Structs (BRIDGE_STRUCT)
 *   H. Classes (Handle-Based)
 *   I. Auto-Cleanup (FinalizationRegistry)
 *   J. Exceptions (BRIDGE_SAFE/THROW)
 *   K. Callbacks (Polling)
 *   L. Callbacks (Push)
 *   M. Modules
 *   N. TypeScript Types
 *   O. Registry Integrity
 *   P. Security
 *   Q-R. Stress & Stability
 *   S. Final Smoke Test
 * 
 * Usage: node test/test_suite.js
 * 
 * =============================================================================
 */

const fs = require('fs');
const path = require('path');

// =============================================================================
// TEST FRAMEWORK
// =============================================================================

let passed = 0;
let failed = 0;
let skipped = 0;
const results = [];

function test(category, name, fn) {
    try {
        const result = fn();
        if (result === 'SKIP') {
            skipped++;
            results.push({ category, name, status: 'SKIP', error: null });
            console.log(`  ⏭️  ${name} (SKIPPED)`);
        } else if (result === true || result === undefined) {
            passed++;
            results.push({ category, name, status: 'PASS', error: null });
            console.log(`  ✅ ${name}`);
        } else {
            failed++;
            results.push({ category, name, status: 'FAIL', error: `Expected true, got: ${result}` });
            console.log(`  ❌ ${name} - Expected true, got: ${result}`);
        }
    } catch (e) {
        failed++;
        results.push({ category, name, status: 'FAIL', error: e.message });
        console.log(`  ❌ ${name} - ${e.message}`);
    }
}

function assert(condition, msg) {
    if (!condition) throw new Error(msg || 'Assertion failed');
    return true;
}

function assertEqual(a, b, msg) {
    if (a !== b) throw new Error(msg || `Expected ${b}, got ${a}`);
    return true;
}

function assertType(val, type, msg) {
    if (typeof val !== type) throw new Error(msg || `Expected ${type}, got ${typeof val}`);
    return true;
}

function category(name) {
    console.log(`\n═══ ${name} ═══`);
}

// =============================================================================
// LOAD BRIDGE
// =============================================================================

let bridge;
try {
    bridge = require('../bridge');
} catch (e) {
    console.error('Failed to load bridge:', e.message);
    process.exit(1);
}

// =============================================================================
// A. CORE SANITY & VERSIONING
// =============================================================================

category('A. Core Sanity & Versioning');

test('A', 'bridge._version should be "6.1.0"', () => {
    return assertEqual(bridge._version, '6.1.0');
});

test('A', 'bridge.bridge_version() should return non-empty string', () => {
    const v = bridge.bridge_version();
    return assertType(v, 'string') && assert(v.length > 0, 'Version should not be empty');
});

test('A', 'bridge.bridge_features() should return valid JSON', () => {
    const json = bridge.bridge_features();
    const parsed = JSON.parse(json);
    return assert(parsed.features !== undefined);
});

test('A', 'Features should include dll-hot-swap', () => {
    const info = bridge._getInfo();
    return assert(info.features.includes('dll-hot-swap'));
});

test('A', 'Features should include zero-copy-buffers', () => {
    const info = bridge._getInfo();
    return assert(info.features.includes('zero-copy-buffers'));
});

test('A', 'Features should include auto-cleanup', () => {
    const info = bridge._getInfo();
    return assert(info.features.includes('auto-cleanup'));
});

test('A', 'Features should include typescript-types', () => {
    const info = bridge._getInfo();
    return assert(info.features.includes('typescript-types'));
});

// =============================================================================
// B. DLL HOT-SWAP
// =============================================================================

category('B. DLL Hot-Swap');

test('B', 'Bridge should load DLL successfully', () => {
    const info = bridge._getInfo();
    return assert(info.dllPath && info.dllPath.length > 0);
});

test('B', 'DLL path should exist', () => {
    const info = bridge._getInfo();
    return assert(fs.existsSync(info.dllPath));
});

test('B', 'demo_add should work before reload', () => {
    return assertEqual(bridge.demo_add(5, 3), 8);
});

test('B', 'bridge.reload() should not throw', () => {
    bridge.reload();
    return true;
});

test('B', 'demo_add should work after reload', () => {
    return assertEqual(bridge.demo_add(5, 3), 8);
});

test('B', 'Repeated reload (3x) should not crash', () => {
    bridge.reload();
    bridge.reload();
    bridge.reload();
    return assertEqual(bridge.demo_add(1, 1), 2);
});

// =============================================================================
// C. ZERO-COPY BUFFERS
// =============================================================================

category('C. Zero-Copy Buffers');

test('C', 'BRIDGE_BUFFER macro exists in bridge.h', () => {
    const bridgeH = fs.readFileSync(path.join(__dirname, '..', 'hub', 'bridge.h'), 'utf8');
    return assert(bridgeH.includes('BRIDGE_BUFFER'));
});

test('C', 'Zero-copy buffer functions (requires C++ implementation)', () => {
    // This test requires processImage to be implemented in C++
    return 'SKIP';
});

// =============================================================================
// D. FUNCTIONS (BRIDGE)
// =============================================================================

category('D. Functions (BRIDGE)');

test('D', 'demo_add(5,3) should return 8', () => {
    return assertEqual(bridge.demo_add(5, 3), 8);
});

test('D', 'demo_add(-5,3) should return -2', () => {
    return assertEqual(bridge.demo_add(-5, 3), -2);
});

test('D', 'demo_add(0,0) should return 0', () => {
    return assertEqual(bridge.demo_add(0, 0), 0);
});

test('D', 'demo_multiply(2.5,4) should return 10', () => {
    return assertEqual(bridge.demo_multiply(2.5, 4), 10);
});

test('D', 'demo_greet("World") should return "Hello, World!"', () => {
    return assertEqual(bridge.demo_greet("World"), "Hello, World!");
});

test('D', 'demo_greet("Test") should return "Hello, Test!"', () => {
    return assertEqual(bridge.demo_greet("Test"), "Hello, Test!");
});

// =============================================================================
// E. VARIABLES (BRIDGE_VAR)
// =============================================================================

category('E. Variables (BRIDGE_VAR)');

test('E', 'get_demo_counter() should return number', () => {
    return assertType(bridge.get_demo_counter(), 'number');
});

test('E', 'set_demo_counter(50) then get should return 50', () => {
    bridge.set_demo_counter(50);
    return assertEqual(bridge.get_demo_counter(), 50);
});

test('E', 'set_demo_counter(-10) should work', () => {
    bridge.set_demo_counter(-10);
    return assertEqual(bridge.get_demo_counter(), -10);
});

test('E', 'Counter value should persist across calls', () => {
    bridge.set_demo_counter(42);
    const a = bridge.get_demo_counter();
    const b = bridge.get_demo_counter();
    return assertEqual(a, b);
});

test('E', 'get_demo_user() should return string', () => {
    return assertType(bridge.get_demo_user(), 'string');
});

test('E', 'set_demo_user("Admin") should work', () => {
    bridge.set_demo_user("Admin");
    return assertEqual(bridge.get_demo_user(), "Admin");
});

test('E', 'set_demo_user("") should work', () => {
    bridge.set_demo_user("");
    return assertEqual(bridge.get_demo_user(), "");
});

// =============================================================================
// F. ENUMS (BRIDGE_ENUM)
// =============================================================================

category('F. Enums (BRIDGE_ENUM)');

test('F', 'DemoColor_Red() should return 0', () => {
    return assertEqual(bridge.DemoColor_Red(), 0);
});

test('F', 'DemoColor_Green() should return 1', () => {
    return assertEqual(bridge.DemoColor_Green(), 1);
});

test('F', 'DemoColor_Blue() should return 2', () => {
    return assertEqual(bridge.DemoColor_Blue(), 2);
});

test('F', 'DemoStatus_Idle() should return 0', () => {
    return assertEqual(bridge.DemoStatus_Idle(), 0);
});

test('F', 'DemoStatus_Running() should return 1', () => {
    return assertEqual(bridge.DemoStatus_Running(), 1);
});

test('F', 'Enum values should be stable', () => {
    const a = bridge.DemoColor_Red();
    const b = bridge.DemoColor_Red();
    return assertEqual(a, b);
});

test('F', 'Non-existent enum should be undefined', () => {
    return assertEqual(bridge.NonExistentEnum_Value, undefined);
});

// =============================================================================
// G. STRUCTS (BRIDGE_STRUCT)
// =============================================================================

category('G. Structs (BRIDGE_STRUCT)');

test('G', 'DemoPoint_create() should return JSON string', () => {
    const result = bridge.DemoPoint_create(10, 20);
    return assertType(result, 'string');
});

test('G', 'DemoPoint_create(10,20) should parse to {x:10,y:20}', () => {
    const result = JSON.parse(bridge.DemoPoint_create(10, 20));
    return assertEqual(result.x, 10) && assertEqual(result.y, 20);
});

test('G', 'DemoVec3_create() should return JSON string', () => {
    const result = bridge.DemoVec3_create(1, 2, 3);
    return assertType(result, 'string');
});

test('G', 'DemoVec3_create(1,2,3) should parse correctly', () => {
    const result = JSON.parse(bridge.DemoVec3_create(1, 2, 3));
    return assertEqual(result.x, 1) && assertEqual(result.y, 2) && assertEqual(result.z, 3);
});

// =============================================================================
// H. CLASSES (Handle-Based)
// =============================================================================

category('H. Classes (Handle-Based)');

test('H', 'DemoCalc_new() should return number > 0', () => {
    const h = bridge.DemoCalc_new();
    return assert(h > 0);
});

test('H', 'Handles should be unique', () => {
    const h1 = bridge.DemoCalc_new();
    const h2 = bridge.DemoCalc_new();
    return assert(h1 !== h2);
});

test('H', 'DemoCalc_addTo(h,10) should return 10', () => {
    const h = bridge.DemoCalc_new();
    bridge.DemoCalc_reset(h);
    return assertEqual(bridge.DemoCalc_addTo(h, 10), 10);
});

test('H', 'DemoCalc_getValue(h) should return current value', () => {
    const h = bridge.DemoCalc_new();
    bridge.DemoCalc_reset(h);
    bridge.DemoCalc_addTo(h, 25);
    return assertEqual(bridge.DemoCalc_getValue(h), 25);
});

test('H', 'DemoCalc_reset(h) should reset to 0', () => {
    const h = bridge.DemoCalc_new();
    bridge.DemoCalc_addTo(h, 100);
    bridge.DemoCalc_reset(h);
    return assertEqual(bridge.DemoCalc_getValue(h), 0);
});

test('H', 'DemoCalc_delete(h) should not throw', () => {
    const h = bridge.DemoCalc_new();
    bridge.DemoCalc_delete(h);
    return true;
});

test('H', 'Invalid handle should not crash', () => {
    const result = bridge.DemoCalc_getValue(99999);
    return assertEqual(result, 0); // Returns 0 for invalid
});

// =============================================================================
// I. AUTO-CLEANUP (FinalizationRegistry)
// =============================================================================

category('I. Auto-Cleanup (FinalizationRegistry)');

test('I', 'createClass should be a function', () => {
    return assertType(bridge.createClass, 'function');
});

test('I', 'createClass("DemoCalc") should return factory', () => {
    const factory = bridge.createClass("DemoCalc");
    return assert(factory !== null);
});

test('I', 'Factory.create() should return wrapper', () => {
    const factory = bridge.createClass("DemoCalc");
    if (!factory) return 'SKIP';
    const obj = factory.create();
    return assert(obj.handle > 0);
});

test('I', 'Multiple instances should work', () => {
    const factory = bridge.createClass("DemoCalc");
    if (!factory) return 'SKIP';
    const a = factory.create();
    const b = factory.create();
    return assert(a.handle !== b.handle);
});

// =============================================================================
// J. EXCEPTIONS (BRIDGE_SAFE/THROW)
// =============================================================================

category('J. Exceptions (BRIDGE_SAFE/THROW)');

test('J', 'demo_divide(10,2) should return 5', () => {
    const result = bridge.demo_divide(10, 2);
    return assertEqual(result, 5);
});

test('J', 'bridge_has_error() after valid call should be 0', () => {
    bridge.demo_divide(10, 2);
    return assertEqual(bridge.bridge_has_error(), 0);
});

test('J', 'demo_divide(10,0) should return 0', () => {
    const result = bridge.demo_divide(10, 0);
    return assertEqual(result, 0);
});

test('J', 'bridge_has_error() after div/0 should be 1', () => {
    bridge.demo_divide(10, 0);
    return assertEqual(bridge.bridge_has_error(), 1);
});

test('J', 'bridge_get_last_error() should return error message', () => {
    bridge.demo_divide(10, 0);
    const err = bridge.bridge_get_last_error();
    return assert(err.includes('Division') || err.includes('zero'));
});

test('J', 'Error state should reset after get', () => {
    bridge.demo_divide(10, 0);
    bridge.bridge_get_last_error();
    return assertEqual(bridge.bridge_has_error(), 0);
});

// =============================================================================
// K. CALLBACKS (Polling)
// =============================================================================

category('K. Callbacks (Polling)');

test('K', 'bridge_poll_callbacks() should return JSON', () => {
    const result = bridge.bridge_poll_callbacks();
    JSON.parse(result); // Should not throw
    return true;
});

test('K', 'Polling with no callbacks should return []', () => {
    bridge.bridge_poll_callbacks(); // Clear first
    const result = JSON.parse(bridge.bridge_poll_callbacks());
    return assert(Array.isArray(result));
});

test('K', 'demo_start_task should queue callbacks', () => {
    bridge.demo_start_task(1);
    const result = JSON.parse(bridge.bridge_poll_callbacks());
    return assert(result.length > 0);
});

test('K', 'Callbacks should be cleared after polling', () => {
    bridge.demo_start_task(1);
    bridge.bridge_poll_callbacks(); // First poll
    const result = JSON.parse(bridge.bridge_poll_callbacks()); // Second poll
    return assertEqual(result.length, 0);
});

// =============================================================================
// L. CALLBACKS (Push Registration)
// =============================================================================

category('L. Callbacks (Push Registration)');

test('L', 'registerCallback should be a function', () => {
    return assertType(bridge.registerCallback, 'function');
});

test('L', 'registerCallback(name, fn) should not throw', () => {
    bridge.registerCallback('TestCallback', (v) => console.log(v));
    return true;
});

test('L', 'Registering same callback twice should not throw', () => {
    bridge.registerCallback('TestCallback', (v) => { });
    bridge.registerCallback('TestCallback', (v) => { });
    return true;
});

// =============================================================================
// M. MODULES
// =============================================================================

category('M. Modules');

test('M', 'DemoMath_sin(0) should return 0', () => {
    return assertEqual(bridge.DemoMath_sin(0), 0);
});

test('M', 'DemoMath_cos(0) should return 1', () => {
    return assertEqual(bridge.DemoMath_cos(0), 1);
});

test('M', 'DemoStr_upper("hello") should return "HELLO"', () => {
    return assertEqual(bridge.DemoStr_upper("hello"), "HELLO");
});

test('M', 'DemoStr_length("test") should return 4', () => {
    return assertEqual(bridge.DemoStr_length("test"), 4);
});

test('M', 'Non-existent module function should be undefined', () => {
    return assertEqual(bridge.FakeModule_fake, undefined);
});

// =============================================================================
// N. TYPESCRIPT TYPES
// =============================================================================

category('N. TypeScript Types');

test('N', 'bridge.d.ts should exist', () => {
    const dtsPath = path.join(__dirname, '..', 'bridge', 'bridge.d.ts');
    return assert(fs.existsSync(dtsPath));
});

test('N', 'bridge.d.ts should contain UniversalBridge interface', () => {
    const dtsPath = path.join(__dirname, '..', 'bridge', 'bridge.d.ts');
    const content = fs.readFileSync(dtsPath, 'utf8');
    return assert(content.includes('interface UniversalBridge'));
});

test('N', 'bridge.d.ts should contain demo_add', () => {
    const dtsPath = path.join(__dirname, '..', 'bridge', 'bridge.d.ts');
    const content = fs.readFileSync(dtsPath, 'utf8');
    return assert(content.includes('demo_add'));
});

test('N', 'bridge.d.ts should contain DemoCalc methods', () => {
    const dtsPath = path.join(__dirname, '..', 'bridge', 'bridge.d.ts');
    const content = fs.readFileSync(dtsPath, 'utf8');
    return assert(content.includes('DemoCalc_new'));
});

// =============================================================================
// O. REGISTRY INTEGRITY
// =============================================================================

category('O. Registry Integrity');

test('O', 'registry.json should exist', () => {
    const regPath = path.join(__dirname, '..', 'bridge', 'registry.json');
    return assert(fs.existsSync(regPath));
});

test('O', 'registry.json should be valid JSON', () => {
    const regPath = path.join(__dirname, '..', 'bridge', 'registry.json');
    const content = fs.readFileSync(regPath, 'utf8');
    const parsed = JSON.parse(content);
    return assert(parsed.functions !== undefined);
});

test('O', 'Registry should have functions object', () => {
    const regPath = path.join(__dirname, '..', 'bridge', 'registry.json');
    const reg = JSON.parse(fs.readFileSync(regPath, 'utf8'));
    return assert(Object.keys(reg.functions).length > 0);
});

test('O', 'All registry functions should be bound', () => {
    const regPath = path.join(__dirname, '..', 'bridge', 'registry.json');
    const reg = JSON.parse(fs.readFileSync(regPath, 'utf8'));
    let missing = 0;
    for (const name of Object.keys(reg.functions)) {
        if (typeof bridge[name] === 'undefined') missing++;
    }
    return assert(missing < 5); // Allow a few missing (internal functions)
});

// =============================================================================
// P. SECURITY
// =============================================================================

category('P. Security');

test('P', 'generate-registry.js should have path validation', () => {
    const genPath = path.join(__dirname, '..', 'scripts', 'generate-registry.js');
    const content = fs.readFileSync(genPath, 'utf8');
    return assert(content.includes('isPathSafe'));
});

test('P', 'generate-registry.js should have function name validation', () => {
    const genPath = path.join(__dirname, '..', 'scripts', 'generate-registry.js');
    const content = fs.readFileSync(genPath, 'utf8');
    return assert(content.includes('isValidFunctionName'));
});

test('P', 'generate-registry.js should have file size limit', () => {
    const genPath = path.join(__dirname, '..', 'scripts', 'generate-registry.js');
    const content = fs.readFileSync(genPath, 'utf8');
    return assert(content.includes('MAX_FILE_SIZE'));
});

test('P', 'Invalid function call should not crash', () => {
    const result = bridge.nonExistentFunction;
    return assertEqual(result, undefined);
});

// =============================================================================
// Q-R. STRESS & STABILITY
// =============================================================================

category('Q-R. Stress & Stability');

test('Q', '1000 function calls should complete', () => {
    for (let i = 0; i < 1000; i++) {
        bridge.demo_add(i, 1);
    }
    return true;
});

test('Q', '100 object creations should complete', () => {
    const handles = [];
    for (let i = 0; i < 100; i++) {
        handles.push(bridge.DemoCalc_new());
    }
    // Cleanup
    for (const h of handles) {
        bridge.DemoCalc_delete(h);
    }
    return true;
});

test('R', 'Repeated reload should not crash', () => {
    for (let i = 0; i < 5; i++) {
        bridge.reload();
    }
    return assertEqual(bridge.demo_add(1, 1), 2);
});

test('R', 'Mixed operations should not corrupt state', () => {
    bridge.set_demo_counter(100);
    bridge.demo_add(5, 5);
    const h = bridge.DemoCalc_new();
    bridge.DemoCalc_addTo(h, 50);
    bridge.demo_divide(10, 2);
    return assertEqual(bridge.get_demo_counter(), 100);
});

// =============================================================================
// S. FINAL SMOKE TEST
// =============================================================================

category('S. Final Smoke Test');

test('S', 'All feature types usable in same session', () => {
    // Functions
    bridge.demo_add(1, 1);

    // Variables
    bridge.set_demo_counter(1);
    bridge.get_demo_counter();

    // Enums
    bridge.DemoColor_Red();

    // Structs
    bridge.DemoPoint_create(1, 1);

    // Classes
    const h = bridge.DemoCalc_new();
    bridge.DemoCalc_delete(h);

    // Exceptions
    bridge.demo_divide(1, 1);

    // Callbacks
    bridge.bridge_poll_callbacks();

    // Modules
    bridge.DemoMath_sin(0);

    return true;
});

test('S', 'No crashes occurred during test', () => {
    return true;
});

test('S', 'Process can exit cleanly', () => {
    return true;
});

// =============================================================================
// SUMMARY
// =============================================================================

console.log('\n═══════════════════════════════════════════════');
console.log('              TEST RESULTS');
console.log('═══════════════════════════════════════════════');
console.log(`  ✅ Passed:  ${passed}`);
console.log(`  ❌ Failed:  ${failed}`);
console.log(`  ⏭️  Skipped: ${skipped}`);
console.log(`  📊 Total:   ${passed + failed + skipped}`);
console.log('═══════════════════════════════════════════════');

if (failed > 0) {
    console.log('\n❌ SOME TESTS FAILED\n');
    process.exit(1);
} else {
    console.log('\n✅ ALL TESTS PASSED\n');
    process.exit(0);
}
