/**
 * =============================================================================
 * BUILD.JS - CPPBridge v3.1 'Robotic' Build System
 * =============================================================================
 * 
 * Usage: npm run build:bridge
 * 
 * Features:
 * 1. Auto-locates Visual Studio / MSVC compiler
 * 2. Parses C++ files to find BRIDGE_FN() declarations
 * 3. Generates registry.json for auto-discovery
 * 4. Generates index.d.ts for TypeScript/IntelliSense
 * 5. Compiles C++ to DLL with friendly error messages
 * 
 * =============================================================================
 */

const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

// =============================================================================
// CONFIGURATION
// =============================================================================

const CONFIG = {
    srcDir: path.join(__dirname, '..', 'src'),
    outputDir: path.join(__dirname, '..', 'bridge'),
    headerFile: path.join(__dirname, '..', 'bridge_core.h'),
    registryFile: path.join(__dirname, '..', 'bridge', 'registry.json'),
    typesFile: path.join(__dirname, '..', 'bridge', 'index.d.ts'),
    dllName: 'cppbridge',
};

// =============================================================================
// MSVC AUTO-DETECTION
// =============================================================================

function findMSVC() {
    console.log('🔍 Searching for Visual Studio...');

    // Try vswhere first
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
                    console.log(`   ✅ Found: ${result}`);
                    return vcvarsPath;
                }
            } catch (e) {
                // Continue to fallback
            }
        }
    }

    // Fallback: Try common paths
    const directPaths = [
        'C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat',
        'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Auxiliary\\Build\\vcvars64.bat',
        'C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Auxiliary\\Build\\vcvars64.bat',
        'C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat',
        'C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat',
        'C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Professional\\VC\\Auxiliary\\Build\\vcvars64.bat',
    ];

    for (const p of directPaths) {
        if (fs.existsSync(p)) {
            const vsPath = path.dirname(path.dirname(path.dirname(p)));
            console.log(`   ✅ Found: ${vsPath}`);
            return p;
        }
    }

    throw new Error('❌ Visual Studio not found! Please install Visual Studio with C++ Desktop Development tools.');
}

// =============================================================================
// C++ PARSER (Extract BRIDGE_FN declarations)
// =============================================================================

function parseBridgeFunctions(srcDir) {
    console.log('📖 Scanning C++ files...');

    const functions = [];
    const cppFiles = [];

    // Find .cpp files in src/
    if (fs.existsSync(srcDir)) {
        for (const file of fs.readdirSync(srcDir)) {
            if (file.endsWith('.cpp')) {
                cppFiles.push(path.join(srcDir, file));
            }
        }
    }

    // Also check root
    const rootDir = path.join(__dirname, '..');
    for (const file of fs.readdirSync(rootDir)) {
        if (file.endsWith('.cpp') && file !== 'main.cpp') {
            cppFiles.push(path.join(rootDir, file));
        }
    }

    for (const filePath of cppFiles) {
        console.log(`   📄 ${path.basename(filePath)}`);
        const content = fs.readFileSync(filePath, 'utf8');
        const lines = content.split('\n');

        for (const line of lines) {
            const trimmed = line.trim();

            // Skip comments
            if (trimmed.startsWith('//') || trimmed.startsWith('/*') || trimmed.startsWith('*')) {
                continue;
            }

            // Match BRIDGE_FN(type, name, args...) or BRIDGE_FN_SAFE(type, name, default, args...)
            let match = line.match(/BRIDGE_FN\s*\(\s*([a-zA-Z_*\s]+?)\s*,\s*(\w+)\s*(?:,\s*(.+?))?\s*\)/);
            if (!match) {
                match = line.match(/BRIDGE_FN_SAFE\s*\(\s*([a-zA-Z_*\s]+?)\s*,\s*(\w+)\s*,\s*[^,]+\s*(?:,\s*(.+?))?\s*\)/);
            }

            if (match) {
                const returnType = match[1].trim();
                const funcName = match[2].trim();
                const argsString = match[3] ? match[3].trim() : '';

                const params = parseArgs(argsString);

                functions.push({
                    name: funcName,
                    returnType: normalizeType(returnType),
                    params: params.map(p => ({
                        type: normalizeType(p.type),
                        name: p.name
                    }))
                });

                console.log(`      ✅ ${funcName}(${params.map(p => p.name).join(', ')}) -> ${returnType}`);
            }
        }
    }

    console.log(`   📊 Found ${functions.length} functions`);
    return { functions, cppFiles };
}

function parseArgs(argsString) {
    const params = [];
    if (!argsString) return params;

    const argParts = argsString.split(',').map(s => s.trim());
    for (const arg of argParts) {
        const parts = arg.trim().split(/\s+/);
        if (parts.length >= 2) {
            let name = parts.pop();
            // Remove leading * from pointer parameter names (e.g., "*name" -> "name")
            if (name.startsWith('*')) {
                name = name.substring(1);
            }
            const type = parts.join(' ');
            params.push({ type, name });
        } else if (parts.length === 1) {
            params.push({ type: parts[0], name: 'arg' + params.length });
        }
    }
    return params;
}

function normalizeType(cppType) {
    const typeMap = {
        'int': 'int', 'float': 'float', 'double': 'double',
        'bool': 'bool', 'void': 'void',
        'char*': 'str', 'const char*': 'str',
        'char *': 'str', 'const char *': 'str', 'const char': 'str',
    };
    const normalized = cppType.replace(/\s+/g, ' ').trim();
    if (typeMap[normalized]) return typeMap[normalized];
    if (normalized.includes('char*') || normalized.includes('char *')) return 'str';
    return normalized;
}

// =============================================================================
// GENERATE REGISTRY.JSON
// =============================================================================

function generateRegistry(functions) {
    console.log('📝 Generating registry.json...');

    const registry = {
        version: '3.1.0',
        generated: new Date().toISOString(),
        functions: {}
    };

    for (const fn of functions) {
        registry.functions[fn.name] = {
            returnType: fn.returnType,
            params: fn.params.map(p => p.type)
        };
    }

    if (!fs.existsSync(CONFIG.outputDir)) {
        fs.mkdirSync(CONFIG.outputDir, { recursive: true });
    }

    fs.writeFileSync(CONFIG.registryFile, JSON.stringify(registry, null, 2));
    console.log(`   ✅ Written: registry.json`);

    return registry;
}

// =============================================================================
// GENERATE TYPESCRIPT DEFINITIONS (IntelliSense)
// =============================================================================

function generateTypeScript(functions) {
    console.log('📝 Generating index.d.ts...');

    const tsTypeMap = {
        'int': 'number', 'float': 'number', 'double': 'number',
        'bool': 'boolean', 'void': 'void', 'str': 'string',
    };

    function toTsType(type) {
        return tsTypeMap[type] || 'any';
    }

    let dts = `// Auto-generated by CPPBridge v3.1\n`;
    dts += `// Do not edit manually - run 'npm run build:bridge' to regenerate\n\n`;
    dts += `declare const bridge: {\n`;

    for (const fn of functions) {
        const returnTs = toTsType(fn.returnType);
        const paramsTs = fn.params.map(p => `${p.name}: ${toTsType(p.type)}`).join(', ');
        dts += `  ${fn.name}(${paramsTs}): ${returnTs};\n`;
    }

    dts += `\n  // Utility functions\n`;
    dts += `  reload(): typeof bridge;\n`;
    dts += `  _getInfo(): { version: string; environment: string; loadedFunctions: string[] };\n`;
    dts += `  _version: string;\n`;
    dts += `  _environment: string;\n`;
    dts += `};\n\n`;
    dts += `export = bridge;\n`;

    fs.writeFileSync(CONFIG.typesFile, dts);
    console.log(`   ✅ Written: index.d.ts`);
}

// =============================================================================
// COMPILE C++
// =============================================================================

function compile(vcvarsPath, cppFiles) {
    console.log('🔨 Compiling...');

    if (cppFiles.length === 0) {
        console.log('   ⚠️  No C++ files found in src/');
        console.log('   💡 Create src/main.cpp with BRIDGE_FN declarations');
        return false;
    }

    const outputDll = path.join(CONFIG.outputDir, `${CONFIG.dllName}.dll`);
    const sourceFiles = cppFiles.map(f => `"${f}"`).join(' ');
    const headerDir = path.dirname(CONFIG.headerFile);

    const compileCmd = `cl /LD /EHsc /O2 /std:c++17 /I"${headerDir}" ${sourceFiles} /Fe:"${outputDll}"`;
    const fullCmd = `"${vcvarsPath}" && ${compileCmd}`;

    try {
        console.log(`   📂 Output: ${CONFIG.dllName}.dll`);

        execSync(`cmd /c "${fullCmd}"`, {
            stdio: 'pipe',
            encoding: 'utf8',
            cwd: CONFIG.outputDir
        });

        // Cleanup .obj, .exp, .lib
        for (const ext of ['.obj', '.exp', '.lib']) {
            try {
                const files = fs.readdirSync(CONFIG.outputDir);
                for (const f of files) {
                    if (f.endsWith(ext)) {
                        fs.unlinkSync(path.join(CONFIG.outputDir, f));
                    }
                }
            } catch (e) { }
        }

        console.log('   ✅ Compilation successful!');
        return true;

    } catch (error) {
        console.error('\n❌ Compilation failed!\n');
        const output = error.stdout || error.stderr || error.message;

        if (output.includes('C2065')) {
            console.error('💡 Undeclared identifier - check variable names');
        } else if (output.includes('C2143')) {
            console.error('💡 Syntax error - check for missing ; or }');
        } else if (output.includes('LNK')) {
            console.error('💡 Linker error - function declared but not defined');
        } else {
            console.error(output.slice(0, 800));
        }
        return false;
    }
}

// =============================================================================
// MAIN
// =============================================================================

async function main() {
    console.log('\n╔════════════════════════════════════════════════════════════╗');
    console.log('║         CPPBridge v3.1 - Zero-Config Build System          ║');
    console.log('╚════════════════════════════════════════════════════════════╝\n');

    try {
        const vcvarsPath = findMSVC();
        const { functions, cppFiles } = parseBridgeFunctions(CONFIG.srcDir);

        if (functions.length > 0) {
            generateRegistry(functions);
            generateTypeScript(functions);
        }

        const success = compile(vcvarsPath, cppFiles);

        if (success) {
            console.log('\n╔════════════════════════════════════════════════════════════╗');
            console.log('║                    ✅ BUILD COMPLETE!                       ║');
            console.log('╚════════════════════════════════════════════════════════════╝');
            console.log('\nUsage in JavaScript:\n');
            console.log("   const bridge = require('./bridge');");
            for (const fn of functions) {
                const args = fn.params.map(p => p.name).join(', ');
                console.log(`   bridge.${fn.name}(${args});`);
            }
            console.log('');
        }

    } catch (error) {
        console.error(`\n❌ ${error.message}\n`);
        process.exit(1);
    }
}

main();
