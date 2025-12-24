/**
 * =============================================================================
 * BUILD-WASM.JS - CPPBridge v5.0 WebAssembly Build Script
 * =============================================================================
 * 
 * Compiles C++ to WebAssembly for browsers
 * 
 * Prerequisites:
 *   - Emscripten SDK installed (https://emscripten.org)
 *   - EMSDK environment activated
 * 
 * Usage:
 *   npm run build:web
 * 
 * =============================================================================
 */

const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

// =============================================================================
// CONFIGURATION
// =============================================================================

const BRIDGE_ROOT = path.join(__dirname, '..');
const CONFIG = {
    outputDir: path.join(BRIDGE_ROOT, 'web'),
    outputName: 'cppbridge',
};

// =============================================================================
// FIND EMSCRIPTEN
// =============================================================================

function findEmscripten() {
    console.log('🔍 Finding Emscripten...');

    // Check if emcc is in PATH
    try {
        const version = execSync('emcc --version', { encoding: 'utf8', stdio: 'pipe' });
        console.log(`   ✅ Found: ${version.split('\n')[0]}`);
        return 'emcc';
    } catch (e) { }

    // Check EMSDK environment
    if (process.env.EMSDK) {
        const emcc = path.join(process.env.EMSDK, 'upstream', 'emscripten', 'emcc');
        if (fs.existsSync(emcc) || fs.existsSync(emcc + '.bat')) {
            console.log(`   ✅ Found in EMSDK`);
            return emcc;
        }
    }

    // Check common paths
    const commonPaths = [
        'C:\\emsdk\\upstream\\emscripten\\emcc.bat',
        path.join(process.env.HOME || '', 'emsdk', 'upstream', 'emscripten', 'emcc'),
        '/usr/local/emsdk/upstream/emscripten/emcc',
    ];

    for (const p of commonPaths) {
        if (fs.existsSync(p)) {
            console.log(`   ✅ Found: ${p}`);
            return p;
        }
    }

    throw new Error(
        'Emscripten not found!\n' +
        'Install: https://emscripten.org/docs/getting_started/downloads.html\n' +
        'Then run: emsdk activate latest'
    );
}

// =============================================================================
// FIND SOURCE FILE
// =============================================================================

function findSourceFile() {
    // Check for common names
    const candidates = [
        path.join(BRIDGE_ROOT, 'backend.cpp'),
        path.join(BRIDGE_ROOT, 'my_logic.cpp'),
        path.join(BRIDGE_ROOT, 'logic.cpp'),
        path.join(BRIDGE_ROOT, 'main.cpp'),
    ];

    // Check command line args
    const args = process.argv.slice(2);
    for (let i = 0; i < args.length; i++) {
        if (args[i] === '--target' && args[i + 1]) {
            return path.resolve(args[++i]);
        }
    }

    // Auto-detect
    for (const p of candidates) {
        if (fs.existsSync(p)) {
            return p;
        }
    }

    // Check examples folder
    const examples = path.join(BRIDGE_ROOT, 'examples');
    if (fs.existsSync(examples)) {
        const files = fs.readdirSync(examples).filter(f => f.endsWith('.cpp'));
        if (files.length > 0) {
            return path.join(examples, files[0]);
        }
    }

    throw new Error('No C++ source file found! Create backend.cpp or use --target');
}

// =============================================================================
// BUILD
// =============================================================================

function build(emcc, sourceFile) {
    console.log('🔨 Compiling to WebAssembly...');
    console.log(`   📄 Source: ${path.basename(sourceFile)}`);

    // Create output directory
    if (!fs.existsSync(CONFIG.outputDir)) {
        fs.mkdirSync(CONFIG.outputDir, { recursive: true });
    }

    const outputJs = path.join(CONFIG.outputDir, `${CONFIG.outputName}.js`);
    const outputWasm = path.join(CONFIG.outputDir, `${CONFIG.outputName}.wasm`);

    // Emscripten flags
    const flags = [
        '-O2',                          // Optimization
        '-std=c++17',                   // C++17
        `-I"${BRIDGE_ROOT}"`,          // Include bridge header
        '-s WASM=1',                    // Output WebAssembly
        '-s EXPORTED_RUNTIME_METHODS=["ccall","cwrap"]',
        '-s ALLOW_MEMORY_GROWTH=1',     // Dynamic memory
        '-s MODULARIZE=1',              // ES module
        `-s EXPORT_NAME="${CONFIG.outputName}"`,
        '--bind',                       // Enable embind
    ];

    const cmd = `"${emcc}" "${sourceFile}" ${flags.join(' ')} -o "${outputJs}"`;

    try {
        execSync(cmd, { stdio: 'inherit', cwd: BRIDGE_ROOT });

        const jsSize = fs.statSync(outputJs).size;
        const wasmSize = fs.statSync(outputWasm).size;

        console.log(`\n   ✅ ${CONFIG.outputName}.js (${Math.round(jsSize / 1024)} KB)`);
        console.log(`   ✅ ${CONFIG.outputName}.wasm (${Math.round(wasmSize / 1024)} KB)`);

        return true;
    } catch (e) {
        console.error('❌ Build failed!');
        return false;
    }
}

// =============================================================================
// GENERATE HTML LOADER
// =============================================================================

function generateLoader() {
    const html = `<!DOCTYPE html>
<html>
<head>
    <title>CPPBridge WASM Test</title>
</head>
<body>
    <h1>CPPBridge WebAssembly</h1>
    <div id="output"></div>
    
    <script src="${CONFIG.outputName}.js"></script>
    <script>
        ${CONFIG.outputName}().then(bridge => {
            console.log('CPPBridge loaded!');
            window.bridge = bridge;
            
            // Test: call your functions
            // const result = bridge._add(5, 3);
            // document.getElementById('output').textContent = 'Result: ' + result;
        });
    </script>
</body>
</html>`;

    fs.writeFileSync(path.join(CONFIG.outputDir, 'index.html'), html);
    console.log('   📝 index.html (test page)');
}

// =============================================================================
// MAIN
// =============================================================================

function main() {
    console.log('\n╔══════════════════════════════════════════════════════════╗');
    console.log('║       CPPBridge v5.0 - WebAssembly (Browser) Build       ║');
    console.log('╚══════════════════════════════════════════════════════════╝\n');

    try {
        const emcc = findEmscripten();
        const sourceFile = findSourceFile();

        if (build(emcc, sourceFile)) {
            generateLoader();

            console.log('\n╔══════════════════════════════════════════════════════════╗');
            console.log('║                  ✅ WASM BUILD COMPLETE                   ║');
            console.log('╚══════════════════════════════════════════════════════════╝');
            console.log('\nTo test:');
            console.log('   cd web && python -m http.server 8080');
            console.log('   Open: http://localhost:8080\n');
        }
    } catch (e) {
        console.error(`\n❌ ${e.message}\n`);
        process.exit(1);
    }
}

main();
