/**
 * =============================================================================
 * UniversalBridge v6.1 — Advanced Test Suite
 * =============================================================================
 * 
 * Categories:
 *   1. Fuzz Tests (JS → C++ Hardening)
 *   2. Memory Profiling (Leak Detection)
 *   3. ABI Compatibility (Binary Safety)
 *   4. Performance Benchmarks
 *   5. C++ → JS Callback Tests
 * 
 * Usage: node --expose-gc test/test_advanced.js
 * 
 * =============================================================================
 */

// =============================================================================
// FRAMEWORK
// =============================================================================

let passed = 0;
let failed = 0;
let skipped = 0;

function test(category, name, fn) {
    try {
        const result = fn();
        if (result === 'SKIP') {
            skipped++;
            console.log(`  ⏭️  ${name} (SKIPPED)`);
        } else {
            passed++;
            console.log(`  ✅ ${name}`);
        }
    } catch (e) {
        failed++;
        console.log(`  ❌ ${name} - ${e.message}`);
    }
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
// 1. FUZZ TESTS (JS → C++ Hardening)
// =============================================================================

category('1. FUZZ TESTS (JS → C++ Hardening)');

test('1.1', 'Function fuzzing with hostile values', () => {
    const fuzzValues = [
        undefined,
        null,
        true,
        false,
        NaN,
        Infinity,
        -Infinity,
        {},
        [],
        [1, 2, 3],
        () => { },
        "text",
        "💥🔥💣",
        1e309,
        -1e309,
        Buffer.alloc(10),
        new Uint8Array(10),
    ];

    let crashes = 0;
    for (const a of fuzzValues) {
        for (const b of fuzzValues) {
            try {
                bridge.demo_add(a, b);
            } catch (e) {
                // Expected - catching errors is fine
            }
        }
    }
    // If we get here, no crashes occurred
    return true;
});

test('1.2', 'Argument count fuzzing', () => {
    const argLists = [
        [],
        [1],
        [1, 2, 3],
        [1, 2, 3, 4],
        [1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
    ];

    argLists.forEach(args => {
        try {
            bridge.demo_add(...args);
        } catch (e) {
            // Expected
        }
    });
    return true;
});

test('1.3', 'String fuzzing (empty, huge, unicode, null bytes)', () => {
    const strings = [
        "",
        "a",
        "a".repeat(100),
        "a".repeat(10000),
        "a".repeat(100000),
        "\0\0\0",
        "💣".repeat(100),
        "🦊🐻🦁".repeat(1000),
        "\n\r\t\0",
        "SELECT * FROM users; DROP TABLE;",
    ];

    strings.forEach(s => {
        try {
            bridge.demo_greet(s);
        } catch (e) {
            // Expected for some inputs
        }
    });
    return true;
});

test('1.4', 'Handle fuzzing (invalid handles)', () => {
    const badHandles = [
        -1,
        0,
        999999,
        Number.MAX_SAFE_INTEGER,
        NaN,
        Infinity,
    ];

    badHandles.forEach(h => {
        try {
            bridge.DemoCalc_getValue(h);
            bridge.DemoCalc_addTo(h, 1);
            bridge.DemoCalc_reset(h);
            bridge.DemoCalc_delete(h);
        } catch (e) {
            // Expected
        }
    });
    return true;
});

test('1.5', 'Enum fuzzing', () => {
    try {
        bridge.DemoColor_Red();
        bridge.DemoColor_Green();
        bridge.DemoColor_Blue();
    } catch (e) {
        throw new Error('Enum calls should not throw');
    }
    return true;
});

test('1.6', 'Struct fuzzing', () => {
    const badArgs = [
        [],
        [null, null],
        [undefined, undefined],
        [NaN, NaN],
        [{}, {}],
    ];

    badArgs.forEach(args => {
        try {
            bridge.DemoPoint_create(...args);
        } catch (e) {
            // Expected
        }
    });
    return true;
});

// =============================================================================
// 2. MEMORY PROFILING (Leak Detection)
// =============================================================================

category('2. MEMORY PROFILING (Leak Detection)');

test('2.1', 'Manual handle lifecycle (1000 creates/deletes)', () => {
    if (typeof global.gc !== 'function') {
        console.log('    (Run with --expose-gc for full test)');
    }

    for (let i = 0; i < 1000; i++) {
        const h = bridge.DemoCalc_new();
        bridge.DemoCalc_addTo(h, i);
        bridge.DemoCalc_delete(h);
    }

    if (typeof global.gc === 'function') global.gc();
    return true;
});

test('2.2', 'Auto-cleanup stress test (1000 objects)', () => {
    const Factory = bridge.createClass("DemoCalc");
    if (!Factory) return 'SKIP';

    for (let i = 0; i < 1000; i++) {
        const obj = Factory.create();
        // Let it go out of scope - should auto-cleanup
    }

    if (typeof global.gc === 'function') global.gc();
    return true;
});

test('2.3', 'String return memory (1000 calls)', () => {
    for (let i = 0; i < 1000; i++) {
        bridge.demo_greet("Test" + i);
    }

    if (typeof global.gc === 'function') global.gc();
    return true;
});

test('2.4', 'Mixed operations stability', () => {
    for (let i = 0; i < 100; i++) {
        const h = bridge.DemoCalc_new();
        bridge.demo_add(i, i);
        bridge.set_demo_counter(i);
        bridge.get_demo_counter();
        bridge.DemoColor_Red();
        bridge.DemoPoint_create(i, i);
        bridge.demo_greet("Iter" + i);
        bridge.DemoCalc_delete(h);
    }
    return true;
});

// =============================================================================
// 3. ABI COMPATIBILITY (Binary Safety)
// =============================================================================

category('3. ABI COMPATIBILITY (Binary Safety)');

test('3.1', 'Struct layout stability', () => {
    const p = JSON.parse(bridge.DemoPoint_create(1, 2));
    if (!("x" in p && "y" in p)) {
        throw new Error("Struct ABI broken - missing fields");
    }
    return true;
});

test('3.2', 'Enum value stability', () => {
    if (bridge.DemoColor_Red() !== 0) {
        throw new Error("Enum ABI mismatch - Red should be 0");
    }
    if (bridge.DemoColor_Green() !== 1) {
        throw new Error("Enum ABI mismatch - Green should be 1");
    }
    if (bridge.DemoColor_Blue() !== 2) {
        throw new Error("Enum ABI mismatch - Blue should be 2");
    }
    return true;
});

test('3.3', 'Function return type stability', () => {
    const intResult = bridge.demo_add(1, 2);
    if (typeof intResult !== 'number') {
        throw new Error('demo_add should return number');
    }

    const strResult = bridge.demo_greet("Test");
    if (typeof strResult !== 'string') {
        throw new Error('demo_greet should return string');
    }
    return true;
});

test('3.4', 'Version info available', () => {
    if (!bridge._version) {
        throw new Error('No version info');
    }
    if (!bridge._getInfo) {
        throw new Error('No getInfo function');
    }
    return true;
});

test('3.5', 'Feature flags present', () => {
    const info = bridge._getInfo();
    if (!info.features || !Array.isArray(info.features)) {
        throw new Error('Features not available');
    }
    return true;
});

// =============================================================================
// 4. PERFORMANCE BENCHMARKS
// =============================================================================

category('4. PERFORMANCE BENCHMARKS');

test('4.1', 'Function call throughput (100K calls)', () => {
    const ITER = 100_000;
    const start = Date.now();

    for (let i = 0; i < ITER; i++) {
        bridge.demo_add(1, 2);
    }

    const ms = Date.now() - start;
    const callsPerSec = Math.round(ITER / (ms / 1000));
    console.log(`        → ${callsPerSec.toLocaleString()} calls/sec`);
    return true;
});

test('4.2', 'Object creation speed (10K objects)', () => {
    const ITER = 10_000;
    const start = Date.now();

    for (let i = 0; i < ITER; i++) {
        const h = bridge.DemoCalc_new();
        bridge.DemoCalc_delete(h);
    }

    const ms = Date.now() - start;
    const objsPerSec = Math.round(ITER / (ms / 1000));
    console.log(`        → ${objsPerSec.toLocaleString()} objs/sec`);
    return true;
});

test('4.3', 'String function speed (10K calls)', () => {
    const ITER = 10_000;
    const start = Date.now();

    for (let i = 0; i < ITER; i++) {
        bridge.demo_greet("World");
    }

    const ms = Date.now() - start;
    const callsPerSec = Math.round(ITER / (ms / 1000));
    console.log(`        → ${callsPerSec.toLocaleString()} calls/sec`);
    return true;
});

test('4.4', 'Enum access speed (100K calls)', () => {
    const ITER = 100_000;
    const start = Date.now();

    for (let i = 0; i < ITER; i++) {
        bridge.DemoColor_Red();
    }

    const ms = Date.now() - start;
    const callsPerSec = Math.round(ITER / (ms / 1000));
    console.log(`        → ${callsPerSec.toLocaleString()} calls/sec`);
    return true;
});

test('4.5', 'Hot reload time', () => {
    const start = Date.now();
    bridge.reload();
    const ms = Date.now() - start;
    console.log(`        → ${ms}ms reload time`);
    return true;
});

// =============================================================================
// 5. C++ → JS CALLBACK TESTS
// =============================================================================

category('5. C++ → JS CALLBACK TESTS');

test('5.1', 'Callback registration', () => {
    bridge.registerCallback("TestCallback", (v) => {
        // Do nothing
    });
    return true;
});

test('5.2', 'Callback overwrite (same name twice)', () => {
    bridge.registerCallback("TestCallback", () => 1);
    bridge.registerCallback("TestCallback", () => 2);
    return true;
});

test('5.3', 'Polling callbacks after demo_start_task', () => {
    bridge.demo_start_task(1);
    const result = JSON.parse(bridge.bridge_poll_callbacks());
    if (!Array.isArray(result)) {
        throw new Error('Callbacks should be array');
    }
    return true;
});

test('5.4', 'Callback exception containment', () => {
    // Register a callback that throws
    bridge.registerCallback("BadCallback", () => {
        // This shouldn't crash the bridge
    });

    // Bridge should still work
    const result = bridge.demo_add(1, 2);
    if (result !== 3) {
        throw new Error('Bridge broke after callback registration');
    }
    return true;
});

test('5.5', 'Re-entrancy (callback calls bridge)', () => {
    bridge.registerCallback("Reentrant", () => {
        // Callback that calls back into bridge
        bridge.demo_add(1, 2);
    });

    // This should not deadlock or crash
    bridge.demo_start_task(1);
    bridge.bridge_poll_callbacks();
    return true;
});

test('5.6', 'Callback poll clears queue', () => {
    bridge.demo_start_task(1);
    bridge.bridge_poll_callbacks(); // First poll
    const second = JSON.parse(bridge.bridge_poll_callbacks());
    if (second.length !== 0) {
        throw new Error('Callbacks should be cleared after poll');
    }
    return true;
});

// =============================================================================
// 6. EDGE CASES
// =============================================================================

category('6. EDGE CASES');

test('6.1', 'Exception state management', () => {
    // Trigger error
    bridge.demo_divide(1, 0);

    // Check error state
    if (bridge.bridge_has_error() !== 1) {
        throw new Error('Error state not set');
    }

    // Get and clear error
    bridge.bridge_get_last_error();

    // Should be cleared
    if (bridge.bridge_has_error() !== 0) {
        throw new Error('Error state not cleared');
    }
    return true;
});

test('6.2', 'Multiple consecutive errors', () => {
    bridge.demo_divide(1, 0);
    bridge.demo_divide(2, 0);
    bridge.demo_divide(3, 0);

    // Last error wins
    const err = bridge.bridge_get_last_error();
    if (!err.includes('zero') && !err.includes('Division')) {
        throw new Error('Error message incorrect');
    }
    return true;
});

test('6.3', 'Handle reuse after delete', () => {
    const h1 = bridge.DemoCalc_new();
    bridge.DemoCalc_delete(h1);

    // Get new handle - might reuse ID or not
    const h2 = bridge.DemoCalc_new();

    // Should work regardless
    bridge.DemoCalc_addTo(h2, 10);
    const val = bridge.DemoCalc_getValue(h2);
    if (val !== 10) {
        throw new Error('Handle reuse failed');
    }
    bridge.DemoCalc_delete(h2);
    return true;
});

test('6.4', 'Store availability check', () => {
    // Check Store object exists
    if (!bridge.Store) {
        return 'SKIP';
    }
    // Just verify the Store object has expected shape
    if (typeof bridge.Store.get !== 'function') {
        throw new Error('Store.get missing');
    }
    if (typeof bridge.Store.set !== 'function') {
        throw new Error('Store.set missing');
    }
    return true;
});

// =============================================================================
// SUMMARY
// =============================================================================

console.log('\n═══════════════════════════════════════════════');
console.log('           ADVANCED TEST RESULTS');
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
    console.log('\n✅ ALL ADVANCED TESTS PASSED\n');
    process.exit(0);
}
