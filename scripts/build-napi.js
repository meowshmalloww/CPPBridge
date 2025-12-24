/**
 * =============================================================================
 * BUILD-NAPI.JS - CPPBridge N-API Build Script
 * =============================================================================
 * 
 * Compiles C++ to a .node addon using node-gyp
 * 
 * Prerequisites:
 *   npm install -g node-gyp
 *   Windows: Visual Studio Build Tools
 *   macOS: Xcode Command Line Tools
 *   Linux: build-essential
 * 
 * Usage:
 *   npm run build:napi -- --target ./backend.cpp
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

// =============================================================================
// ARGUMENT PARSER
// =============================================================================

function parseArgs() {
    const args = process.argv.slice(2);
    let target = null;

    for (let i = 0; i < args.length; i++) {
        if (args[i] === '--target' && args[i + 1]) {
            target = path.resolve(args[++i]);
        }
    }

    // Auto-detect
    if (!target) {
        const candidates = [
            path.join(BRIDGE_ROOT, 'backend.cpp'),
            path.join(BRIDGE_ROOT, 'my_logic.cpp'),
        ];
        for (const p of candidates) {
            if (fs.existsSync(p)) {
                target = p;
                break;
            }
        }
    }

    return { target };
}

// =============================================================================
// GENERATE BINDING.GYP
// =============================================================================

function generateBindingGyp(targetFile) {
    const gyp = {
        targets: [{
            target_name: 'cppbridge',
            sources: [
                path.relative(BRIDGE_ROOT, targetFile).replace(/\\/g, '/'),
                'napi_wrapper.cpp'
            ],
            include_dirs: [
                "<!@(node -p \"require('node-addon-api').include\")",
                '.'
            ],
            defines: ['NAPI_DISABLE_CPP_EXCEPTIONS'],
            cflags_cc: ['-std=c++17', '-fexceptions'],
            'msvs_settings': {
                'VCCLCompilerTool': {
                    'ExceptionHandling': 1,
                    'AdditionalOptions': ['/std:c++17']
                }
            },
            'xcode_settings': {
                'CLANG_CXX_LANGUAGE_STANDARD': 'c++17',
                'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
            }
        }]
    };

    fs.writeFileSync(
        path.join(BRIDGE_ROOT, 'binding.gyp'),
        JSON.stringify(gyp, null, 2)
    );
    console.log('📝 Generated binding.gyp');
}

// =============================================================================
// GENERATE N-API WRAPPER
// =============================================================================

function generateNapiWrapper(targetFile) {
    // Read and parse the target file for BRIDGE_FN declarations
    const content = fs.readFileSync(targetFile, 'utf8');
    const fnRegex = /BRIDGE_FN\s*\(\s*(\w+(?:\s*\*)?)\s*,\s*(\w+)\s*,?([^)]*)\)/g;

    const functions = [];
    let match;
    while ((match = fnRegex.exec(content)) !== null) {
        functions.push({
            returnType: match[1].trim(),
            name: match[2].trim(),
            params: match[3].trim()
        });
    }

    const wrapper = `
// Auto-generated N-API wrapper
#include <napi.h>
#include "bridge_core.h"

// Forward declarations from user code
${functions.map(f => `extern "C" ${f.returnType} ${f.name}(${f.params});`).join('\n')}

// N-API wrappers
${functions.map(f => generateWrapperFunction(f)).join('\n\n')}

// Init
Napi::Object Init(Napi::Env env, Napi::Object exports) {
${functions.map(f => `    exports.Set("${f.name}", Napi::Function::New(env, _napi_${f.name}));`).join('\n')}
    return exports;
}

NODE_API_MODULE(cppbridge, Init)
`;

    fs.writeFileSync(path.join(BRIDGE_ROOT, 'napi_wrapper.cpp'), wrapper);
    console.log('📝 Generated napi_wrapper.cpp');
    return functions;
}

function generateWrapperFunction(fn) {
    const isString = fn.returnType.includes('char*');
    const isVoid = fn.returnType === 'void';
    const isInt = fn.returnType === 'int';
    const isDouble = fn.returnType === 'double' || fn.returnType === 'float';

    const params = fn.params.split(',').filter(p => p.trim()).map((p, i) => {
        const parts = p.trim().split(/\s+/);
        const type = parts.slice(0, -1).join(' ');
        const name = parts[parts.length - 1].replace('*', '');
        return { type, name, index: i };
    });

    const argConversions = params.map(p => {
        if (p.type.includes('char*')) {
            return `    std::string _${p.name} = info[${p.index}].As<Napi::String>().Utf8Value();
    const char* ${p.name} = _${p.name}.c_str();`;
        } else if (p.type === 'int') {
            return `    int ${p.name} = info[${p.index}].As<Napi::Number>().Int32Value();`;
        } else if (p.type === 'double' || p.type === 'float') {
            return `    double ${p.name} = info[${p.index}].As<Napi::Number>().DoubleValue();`;
        }
        return `    auto ${p.name} = info[${p.index}];`;
    }).join('\n');

    const callArgs = params.map(p => p.name).join(', ');

    let returnStatement;
    if (isVoid) {
        returnStatement = `    ${fn.name}(${callArgs});
    return env.Undefined();`;
    } else if (isString) {
        returnStatement = `    const char* result = ${fn.name}(${callArgs});
    return Napi::String::New(env, result ? result : "");`;
    } else if (isInt) {
        returnStatement = `    int result = ${fn.name}(${callArgs});
    return Napi::Number::New(env, result);`;
    } else if (isDouble) {
        returnStatement = `    double result = ${fn.name}(${callArgs});
    return Napi::Number::New(env, result);`;
    } else {
        returnStatement = `    auto result = ${fn.name}(${callArgs});
    return Napi::Number::New(env, result);`;
    }

    return `Napi::Value _napi_${fn.name}(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
${argConversions}
${returnStatement}
}`;
}

// =============================================================================
// BUILD
// =============================================================================

function build() {
    console.log('🔨 Building N-API addon...');

    try {
        execSync('node-gyp rebuild', {
            cwd: BRIDGE_ROOT,
            stdio: 'inherit'
        });

        // Copy to bridge folder
        const buildPath = path.join(BRIDGE_ROOT, 'build', 'Release', 'cppbridge.node');
        const destPath = path.join(BRIDGE_ROOT, 'bridge', 'cppbridge.node');

        if (fs.existsSync(buildPath)) {
            fs.copyFileSync(buildPath, destPath);
            console.log('✅ cppbridge.node copied to bridge/');
        }

        return true;
    } catch (e) {
        console.error('❌ Build failed!');
        console.log('\nMake sure you have:');
        console.log('  npm install -g node-gyp');
        console.log('  npm install node-addon-api');
        return false;
    }
}

// =============================================================================
// MAIN
// =============================================================================

function main() {
    console.log('\n╔══════════════════════════════════════════════════════════╗');
    console.log('║          CPPBridge v5.0 - N-API Build                    ║');
    console.log('╚══════════════════════════════════════════════════════════╝\n');

    const options = parseArgs();

    if (!options.target) {
        console.error('❌ No source file found!');
        console.log('Usage: npm run build:napi -- --target ./backend.cpp');
        process.exit(1);
    }

    console.log(`📄 Source: ${path.basename(options.target)}`);

    generateBindingGyp(options.target);
    generateNapiWrapper(options.target);

    if (build()) {
        console.log('\n╔══════════════════════════════════════════════════════════╗');
        console.log('║                ✅ N-API BUILD COMPLETE                   ║');
        console.log('╚══════════════════════════════════════════════════════════╝');
        console.log('\nUsage:');
        console.log('  const bridge = require("./bridge/index-napi");');
        console.log('  bridge.yourFunction();');
    }
}

main();
