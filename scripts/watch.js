/**
 * =============================================================================
 * WATCH.JS - CPPBridge v4.0 Live Watcher
 * =============================================================================
 * 
 * Watches your C++ file and auto-rebuilds on save.
 * The app hot-reloads without restart.
 * 
 * Usage:
 *   npm run watch -- --target ./my_logic.cpp
 * 
 * =============================================================================
 */

const fs = require('fs');
const path = require('path');
const { execSync, spawn } = require('child_process');

// =============================================================================
// CONFIGURATION
// =============================================================================

const DEBOUNCE_MS = 300;
const BUILD_SCRIPT = path.join(__dirname, 'build.js');

let debounceTimer = null;
let buildVersion = 0;
let isBuilding = false;

// =============================================================================
// ARGUMENT PARSER
// =============================================================================

function parseArgs() {
    const args = process.argv.slice(2);
    const options = { target: null, output: 'cppbridge' };

    for (let i = 0; i < args.length; i++) {
        if (args[i] === '--target' && args[i + 1]) {
            options.target = path.resolve(args[++i]);
        } else if (args[i] === '--output' && args[i + 1]) {
            options.output = args[++i];
        }
    }

    return options;
}

// =============================================================================
// BUILD FUNCTION
// =============================================================================

function build(targetFile, outputName) {
    if (isBuilding) {
        console.log('⏳ Build already in progress...');
        return;
    }

    isBuilding = true;
    buildVersion++;

    const timestamp = new Date().toLocaleTimeString();
    console.log(`\n🔨 [${timestamp}] Rebuilding... (v${buildVersion})`);

    try {
        // Run build script
        execSync(`node "${BUILD_SCRIPT}" --target "${targetFile}" --output "${outputName}"`, {
            stdio: 'inherit',
            cwd: path.dirname(BUILD_SCRIPT)
        });

        console.log(`✅ Build complete! Hot reload ready.`);
        console.log(`   💡 In your app: bridge.reload()\n`);

    } catch (error) {
        console.error(`❌ Build failed!`);
    }

    isBuilding = false;
}

// =============================================================================
// FILE WATCHER
// =============================================================================

function startWatcher(targetFile, outputName) {
    console.log('\n╔══════════════════════════════════════════════════════════╗');
    console.log('║        CPPBridge v4.0 - Live Watcher Mode                 ║');
    console.log('╚══════════════════════════════════════════════════════════╝\n');

    if (!fs.existsSync(targetFile)) {
        console.error(`❌ File not found: ${targetFile}`);
        process.exit(1);
    }

    console.log(`👁️  Watching: ${path.basename(targetFile)}`);
    console.log(`📂 Output: ${outputName}.dll`);
    console.log(`\n   Edit your C++ file and save. I'll rebuild automatically!\n`);
    console.log('─'.repeat(60));

    // Initial build
    build(targetFile, outputName);

    // Watch for changes
    fs.watch(targetFile, { persistent: true }, (eventType, filename) => {
        if (eventType === 'change') {
            // Debounce: wait for user to stop typing
            if (debounceTimer) {
                clearTimeout(debounceTimer);
            }

            debounceTimer = setTimeout(() => {
                build(targetFile, outputName);
            }, DEBOUNCE_MS);
        }
    });

    // Also watch the directory for renames/moves
    const dir = path.dirname(targetFile);
    fs.watch(dir, { persistent: true }, (eventType, filename) => {
        if (filename === path.basename(targetFile) && eventType === 'rename') {
            console.log(`⚠️  File moved or renamed. Still watching original path.`);
        }
    });

    // Keep process alive
    console.log('\n📡 Watcher running. Press Ctrl+C to stop.\n');
}

// =============================================================================
// MAIN
// =============================================================================

const options = parseArgs();

if (!options.target) {
    console.log('Usage: npm run watch -- --target ./my_logic.cpp');
    console.log('\nExample:');
    console.log('  npm run watch -- --target ./my_logic.cpp');
    console.log('  npm run watch -- --target ../src/game.cpp --output game');
    process.exit(1);
}

startWatcher(options.target, options.output);
