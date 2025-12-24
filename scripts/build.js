/**
 * =============================================================================
 * BUILD.JS - CPPBridge v3.2 'Black Box' Build System
 * =============================================================================
 * 
 * The user NEVER opens the cppbridge folder. They point to their own files.
 * 
 * Usage:
 *   npm run build:bridge -- --target ../my_app.cpp
 *   npm run build:bridge -- --target ./src/logic.cpp
 *   npm run build:bridge                              # (uses internal backend.cpp)
 * 
 * Features:
 *   --target <path>   : Compile ONLY this file (ignores internal files)
 *   --output <name>   : Custom DLL name (default: cppbridge)
 *   --watch           : Watch for changes and auto-rebuild
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
    outputDir: path.join(BRIDGE_ROOT, 'bridge'),
    headerFile: path.join(BRIDGE_ROOT, 'bridge_core.h'),
    registryFile: path.join(BRIDGE_ROOT, 'bridge', 'registry.json'),
    typesFile: path.join(BRIDGE_ROOT, 'bridge', 'index.d.ts'),
    dllName: 'cppbridge',
};

// =============================================================================
// ARGUMENT PARSER
// =============================================================================

function parseArgs() {
    const args = process.argv.slice(2);
    const options = {
        target: null,      // --target <path>
        output: null,      // --output <name>
        watch: false,      // --watch
    };

    for (let i = 0; i < args.length; i++) {
        if (args[i] === '--target' && args[i + 1]) {
            options.target = path.resolve(args[++i]);
        } else if (args[i] === '--output' && args[i + 1]) {
            options.output = args[++i];
        } else if (args[i] === '--watch') {
            options.watch = true;
        }
    }

    // Auto-detect source file if not specified
    if (!options.target) {
        const candidates = [
            path.join(BRIDGE_ROOT, 'backend.cpp'),
            path.join(BRIDGE_ROOT, 'my_logic.cpp'),
            path.join(BRIDGE_ROOT, 'logic.cpp'),
        ];

        for (const p of candidates) {
            if (fs.existsSync(p)) {
                options.target = p;
                break;
            }
        }

        // Check examples folder
        if (!options.target) {
            const examples = path.join(BRIDGE_ROOT, 'examples');
            if (fs.existsSync(examples)) {
                const files = fs.readdirSync(examples).filter(f => f.endsWith('.cpp'));
                if (files.length > 0) {
                    options.target = path.join(examples, files[0]);
                }
            }
        }
    }

    return options;
}

// =============================================================================
// MSVC AUTO-DETECTION
// =============================================================================

function findMSVC() {
    console.log('🔍 Finding Visual Studio...');

    const vswhereLocations = [
        'C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe',
        'C:\\Program Files\\Microsoft Visual Studio\\Installer\\vswhere.exe',
    ];

    for (const vswhere of vswhereLocations) {
        if (fs.existsSync(vswhere)) {
            try {
                const result = execSync(`"${vswhere}" -latest -property installationPath`, { encoding: 'utf8' }).trim();
                const vcvarsPath = path.join(result, 'VC', 'Auxiliary', 'Build', 'vcvars64.bat');
                if (fs.existsSync(vcvarsPath)) {
                    console.log(`   ✅ Found: ${path.basename(result)}`);
                    return vcvarsPath;
                }
            } catch (e) { }
        }
    }

    // Fallback paths
    const directPaths = [
        'C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat',
        'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Auxiliary\\Build\\vcvars64.bat',
        'C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat',
        'C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat',
    ];

    for (const p of directPaths) {
        if (fs.existsSync(p)) {
            console.log(`   ✅ Found: Visual Studio`);
            return p;
        }
    }

    throw new Error('❌ Visual Studio not found! Install Visual Studio with C++ tools.');
}

// =============================================================================
// SOURCE FILE LOCATOR
// =============================================================================

function findSourceFiles(options) {
    console.log('📂 Locating source files...');

    // If --target is specified, ONLY use that file (Black Box mode)
    if (options.target) {
        if (!fs.existsSync(options.target)) {
            throw new Error(`❌ Target file not found: ${options.target}`);
        }
        console.log(`   📄 Target: ${path.basename(options.target)} (external)`);
        return [options.target];
    }

    // Fallback: Look for internal files
    const cppFiles = [];
    const skipFiles = ['main.cpp', 'test.cpp', 'tests.cpp', 'test_all.cpp'];

    // Check for backend.cpp in bridge root
    const backendPath = path.join(BRIDGE_ROOT, 'backend.cpp');
    if (fs.existsSync(backendPath)) {
        cppFiles.push(backendPath);
    }

    // Check src/ folder
    const srcDir = path.join(BRIDGE_ROOT, 'src');
    if (fs.existsSync(srcDir)) {
        for (const file of fs.readdirSync(srcDir)) {
            if (file.endsWith('.cpp') && !skipFiles.includes(file)) {
                cppFiles.push(path.join(srcDir, file));
            }
        }
    }

    for (const f of cppFiles) {
        console.log(`   📄 Found: ${path.basename(f)}`);
    }

    return cppFiles;
}

// =============================================================================
// C++ PARSER
// =============================================================================

function parseBridgeFunctions(cppFiles) {
    console.log('🔎 Scanning for BRIDGE functions...');

    const functions = [];

    for (const filePath of cppFiles) {
        const content = fs.readFileSync(filePath, 'utf8');
        const lines = content.split('\n');

        for (const line of lines) {
            const trimmed = line.trim();
            if (trimmed.startsWith('//') || trimmed.startsWith('/*')) continue;

            // Match BRIDGE_FN, BRIDGE_ASYNC, BRIDGE_JSON
            const patterns = [
                /BRIDGE_FN\s*\(\s*([a-zA-Z_*\s]+?)\s*,\s*(\w+)\s*(?:,\s*(.+?))?\s*\)/,
                /BRIDGE_ASYNC\s*\(\s*([a-zA-Z_*\s]+?)\s*,\s*(\w+)\s*(?:,\s*(.+?))?\s*\)/,
                /BRIDGE_JSON\s*\(\s*(\w+)\s*(?:,\s*(.+?))?\s*\)/,
            ];

            for (const regex of patterns) {
                const match = line.match(regex);
                if (match) {
                    const isAsync = line.includes('BRIDGE_ASYNC');
                    const isJson = line.includes('BRIDGE_JSON');

                    let returnType, funcName, argsString;
                    if (isJson) {
                        returnType = 'str';
                        funcName = match[1].trim();
                        argsString = match[2] || '';
                    } else {
                        returnType = match[1].trim();
                        funcName = match[2].trim();
                        argsString = match[3] || '';
                    }

                    const params = parseArgs2(argsString);

                    functions.push({
                        name: funcName,
                        returnType: normalizeType(returnType),
                        params: params.map(p => ({ type: normalizeType(p.type), name: p.name })),
                        async: isAsync,
                        json: isJson,
                    });

                    const tag = isAsync ? '⚡' : isJson ? '📦' : '✅';
                    console.log(`   ${tag} ${funcName}(${params.map(p => p.name).join(', ')})`);
                    break;
                }
            }
        }
    }

    console.log(`   📊 Total: ${functions.length} functions`);
    return functions;
}

function parseArgs2(argsString) {
    const params = [];
    if (!argsString) return params;

    for (const arg of argsString.split(',').map(s => s.trim())) {
        const parts = arg.split(/\s+/);
        if (parts.length >= 2) {
            let name = parts.pop();
            if (name.startsWith('*')) name = name.substring(1);
            params.push({ type: parts.join(' '), name });
        }
    }
    return params;
}

function normalizeType(cppType) {
    const map = {
        'int': 'int', 'float': 'float', 'double': 'double',
        'bool': 'bool', 'void': 'void',
        'char*': 'str', 'const char*': 'str', 'const char *': 'str',
    };
    const normalized = cppType.replace(/\s+/g, ' ').trim();
    if (map[normalized]) return map[normalized];
    if (normalized.includes('char')) return 'str';
    return normalized;
}

// =============================================================================
// GENERATE REGISTRY & TYPESCRIPT
// =============================================================================

function generateOutputs(functions, dllName) {
    // Ensure output dir exists
    if (!fs.existsSync(CONFIG.outputDir)) {
        fs.mkdirSync(CONFIG.outputDir, { recursive: true });
    }

    // Registry JSON
    const registry = {
        version: '3.2.0',
        generated: new Date().toISOString(),
        dllName: dllName,
        functions: {}
    };

    for (const fn of functions) {
        registry.functions[fn.name] = {
            returnType: fn.returnType,
            params: fn.params.map(p => p.type),
            async: fn.async || false,
        };
    }

    fs.writeFileSync(CONFIG.registryFile, JSON.stringify(registry, null, 2));
    console.log('📝 Generated: registry.json');

    // TypeScript definitions
    const tsMap = { 'int': 'number', 'float': 'number', 'double': 'number', 'bool': 'boolean', 'str': 'string', 'void': 'void' };
    const toTs = (t) => tsMap[t] || 'any';

    let dts = `// Auto-generated by CPPBridge v3.2\n\n`;
    dts += `declare const bridge: {\n`;

    for (const fn of functions) {
        const ret = fn.async ? `Promise<${toTs(fn.returnType)}>` : toTs(fn.returnType);
        const args = fn.params.map(p => `${p.name}: ${toTs(p.type)}`).join(', ');
        dts += `  ${fn.name}(${args}): ${ret};\n`;
    }

    dts += `\n  reload(): typeof bridge;\n`;
    dts += `  _version: string;\n`;
    dts += `};\n\nexport = bridge;\n`;

    fs.writeFileSync(CONFIG.typesFile, dts);
    console.log('📝 Generated: index.d.ts');
}

// =============================================================================
// COMPILE
// =============================================================================

function compile(vcvarsPath, cppFiles, dllName) {
    console.log('🔨 Compiling...');

    if (cppFiles.length === 0) {
        console.log('   ⚠️  No source files found');
        return false;
    }

    const outputDll = path.join(CONFIG.outputDir, `${dllName}.dll`);
    const sources = cppFiles.map(f => `"${f}"`).join(' ');
    const includeDir = BRIDGE_ROOT;

    const cmd = `cl /LD /EHsc /O2 /std:c++17 /I"${includeDir}" ${sources} /Fe:"${outputDll}"`;
    const fullCmd = `"${vcvarsPath}" && ${cmd}`;

    try {
        execSync(`cmd /c "${fullCmd}"`, { stdio: 'pipe', cwd: CONFIG.outputDir });

        // Cleanup
        for (const ext of ['.obj', '.exp', '.lib']) {
            const files = fs.readdirSync(CONFIG.outputDir).filter(f => f.endsWith(ext));
            files.forEach(f => fs.unlinkSync(path.join(CONFIG.outputDir, f)));
        }

        const stats = fs.statSync(outputDll);
        console.log(`   ✅ Built: ${dllName}.dll (${Math.round(stats.size / 1024)} KB)`);
        return true;

    } catch (error) {
        console.error('\n❌ Compilation failed!\n');
        const out = error.stdout || error.stderr || '';

        if (out.includes('C2065')) console.error('💡 Undeclared identifier');
        else if (out.includes('C2143')) console.error('💡 Syntax error');
        else if (out.includes('LNK')) console.error('💡 Linker error');
        else console.error(out.slice(0, 500));

        return false;
    }
}

// =============================================================================
// MAIN
// =============================================================================

async function main() {
    console.log('\n╔══════════════════════════════════════════════════════════╗');
    console.log('║        CPPBridge v3.2 - Black Box Build System           ║');
    console.log('╚══════════════════════════════════════════════════════════╝\n');

    try {
        const options = parseArgs();
        const dllName = options.output || CONFIG.dllName;

        const vcvarsPath = findMSVC();
        const cppFiles = findSourceFiles(options);
        const functions = parseBridgeFunctions(cppFiles);

        if (functions.length > 0) {
            generateOutputs(functions, dllName);
        }

        const success = compile(vcvarsPath, cppFiles, dllName);

        if (success) {
            console.log('\n╔══════════════════════════════════════════════════════════╗');
            console.log('║                   ✅ BUILD COMPLETE                       ║');
            console.log('╚══════════════════════════════════════════════════════════╝');
            console.log('\nUsage:\n');
            console.log("  const bridge = require('./bridge');");
            functions.slice(0, 5).forEach(fn => {
                const args = fn.params.map(p => p.name).join(', ');
                console.log(`  bridge.${fn.name}(${args});`);
            });
            if (functions.length > 5) console.log(`  ... and ${functions.length - 5} more`);
            console.log('');
        }

    } catch (error) {
        console.error(`\n❌ ${error.message}\n`);
        process.exit(1);
    }
}

main();
