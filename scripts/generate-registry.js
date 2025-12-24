/**
 * =============================================================================
 * GENERATE-REGISTRY.JS - Secure Auto-Registry & TypeScript Generator
 * =============================================================================
 * 
 * Features:
 *   - Scans C++ files for BRIDGE functions
 *   - Generates registry.json
 *   - Generates bridge.d.ts for TypeScript autocomplete
 * 
 * Security:
 *   - Path traversal protection
 *   - Function name validation
 *   - File size limits
 * 
 * Usage:
 *   node scripts/generate-registry.js
 * 
 * =============================================================================
 */

const fs = require('fs');
const path = require('path');

// =============================================================================
// SECURITY CONFIGURATION
// =============================================================================

const PROJECT_ROOT = path.resolve(__dirname, '..');
const HUB_DIR = path.join(PROJECT_ROOT, 'hub');
const REGISTRY_PATH = path.join(PROJECT_ROOT, 'bridge', 'registry.json');
const TYPES_PATH = path.join(PROJECT_ROOT, 'bridge', 'bridge.d.ts');

const ALLOWED_EXTENSIONS = ['.cpp', '.h', '.hpp', '.cc', '.cxx'];
const VALID_NAME_REGEX = /^[a-zA-Z_][a-zA-Z0-9_]*$/;
const MAX_FILE_SIZE = 10 * 1024 * 1024;

// =============================================================================
// SECURITY FUNCTIONS
// =============================================================================

function isPathSafe(filePath) {
    const resolved = path.resolve(filePath);
    const normalized = path.normalize(resolved);
    if (!normalized.startsWith(PROJECT_ROOT)) return false;
    if (filePath.includes('\0')) return false;
    return true;
}

function isValidFunctionName(name) {
    if (!name || typeof name !== 'string') return false;
    if (name.length > 128) return false;
    if (!VALID_NAME_REGEX.test(name)) return false;
    return true;
}

// =============================================================================
// TYPE MAPPING
// =============================================================================

const TYPE_MAP = {
    'int': 'int',
    'double': 'double',
    'float': 'double',
    'void': 'void',
    'str': 'str',
    'const char*': 'str',
    'char*': 'str',
    'bool': 'int',
    'uint64_t': 'uint64',
    'int64_t': 'int64',
    'size_t': 'uint64',
};

// TypeScript type mapping
const TS_TYPE_MAP = {
    'int': 'number',
    'double': 'number',
    'float': 'number',
    'void': 'void',
    'str': 'string',
    'uint64': 'bigint',
    'int64': 'bigint',
};

function mapType(cppType) {
    if (!cppType) return 'int';
    cppType = cppType.trim().replace(/\s+/g, ' ');
    return TYPE_MAP[cppType] || 'int';
}

function mapToTS(registryType) {
    return TS_TYPE_MAP[registryType] || 'number';
}

function parseParams(paramsStr) {
    if (!paramsStr || paramsStr.trim() === '' || paramsStr.trim() === 'void') {
        return [];
    }

    const params = [];
    const parts = paramsStr.split(',');

    for (const part of parts) {
        const trimmed = part.trim();
        if (!trimmed) continue;

        const tokens = trimmed.split(/\s+/);
        if (tokens.length >= 2) {
            const type = tokens.slice(0, -1).join(' ');
            const name = tokens[tokens.length - 1].replace(/[*&]/g, '');
            params.push({ type: mapType(type), name });
        } else if (tokens.length === 1) {
            params.push({ type: mapType(tokens[0]), name: `arg${params.length}` });
        }
    }

    return params;
}

// =============================================================================
// FILE SCANNER
// =============================================================================

function scanFile(filePath) {
    if (!isPathSafe(filePath)) return [];

    const ext = path.extname(filePath).toLowerCase();
    if (!ALLOWED_EXTENSIONS.includes(ext)) return [];

    try {
        const stat = fs.statSync(filePath);
        if (stat.size > MAX_FILE_SIZE) return [];
    } catch (e) {
        return [];
    }

    let content;
    try {
        content = fs.readFileSync(filePath, 'utf8');
    } catch (e) {
        return [];
    }

    const functions = [];

    // Match BRIDGE patterns
    const pattern = /(?:BRIDGE|BRIDGE_EXPORT|BRIDGE_SAFE)\s+(\w+(?:\s*\*)?)\s+(\w+)\s*\(([^)]*)\)/g;
    let match;
    while ((match = pattern.exec(content)) !== null) {
        const returnType = match[1].trim();
        const funcName = match[2].trim();
        const paramsStr = match[3];

        if (!isValidFunctionName(funcName)) continue;
        if (funcName.startsWith('_')) continue;

        const params = parseParams(paramsStr);

        functions.push({
            name: funcName,
            returnType: mapType(returnType),
            params: params,
            async: false
        });
    }

    // Match BRIDGE_VAR patterns
    const varPattern = /BRIDGE_VAR\s*\(\s*(\w+)\s*,\s*(\w+)/g;
    while ((match = varPattern.exec(content)) !== null) {
        const type = match[1];
        const name = match[2];
        if (isValidFunctionName(`get_${name}`)) {
            functions.push({ name: `get_${name}`, returnType: mapType(type), params: [], async: false });
            functions.push({ name: `set_${name}`, returnType: 'void', params: [{ type: mapType(type), name: 'value' }], async: false });
        }
    }

    // Match BRIDGE_VAR_STR
    const varStrPattern = /BRIDGE_VAR_STR\s*\(\s*(\w+)/g;
    while ((match = varStrPattern.exec(content)) !== null) {
        const name = match[1];
        if (isValidFunctionName(`get_${name}`)) {
            functions.push({ name: `get_${name}`, returnType: 'str', params: [], async: false });
            functions.push({ name: `set_${name}`, returnType: 'void', params: [{ type: 'str', name: 'value' }], async: false });
        }
    }

    // Match BRIDGE_ENUM values
    const enumPattern = /BRIDGE_ENUM_\d+\s*\(\s*(\w+)\s*,([^)]+)\)/g;
    while ((match = enumPattern.exec(content)) !== null) {
        const enumName = match[1];
        const values = match[2].split(',').map(v => v.trim());
        for (const value of values) {
            const name = `${enumName}_${value}`;
            if (isValidFunctionName(name)) {
                functions.push({ name, returnType: 'int', params: [], async: false });
            }
        }
    }

    // Match BRIDGE_STRUCT
    const structPattern = /BRIDGE_STRUCT_(\d+)\s*\(\s*(\w+)/g;
    while ((match = structPattern.exec(content)) !== null) {
        const structName = match[2];
        const name = `${structName}_create`;
        if (isValidFunctionName(name)) {
            functions.push({ name, returnType: 'str', params: [], async: false });
        }
    }

    return functions;
}

function scanDirectory(dir) {
    if (!isPathSafe(dir)) return [];

    const allFunctions = [];

    function walk(currentDir) {
        if (!isPathSafe(currentDir)) return;

        let files;
        try {
            files = fs.readdirSync(currentDir);
        } catch (e) {
            return;
        }

        for (const file of files) {
            if (file.startsWith('.') || file === 'node_modules' || file === 'build') {
                continue;
            }

            const filePath = path.join(currentDir, file);
            if (!isPathSafe(filePath)) continue;

            let stat;
            try {
                stat = fs.statSync(filePath);
            } catch (e) {
                continue;
            }

            if (stat.isDirectory()) {
                walk(filePath);
            } else if (stat.isFile()) {
                const ext = path.extname(file).toLowerCase();
                if (ALLOWED_EXTENSIONS.includes(ext)) {
                    const functions = scanFile(filePath);
                    allFunctions.push(...functions);
                }
            }
        }
    }

    walk(dir);
    return allFunctions;
}

// =============================================================================
// TYPESCRIPT GENERATOR
// =============================================================================

function generateTypeScript(functions) {
    let ts = `/**
 * UniversalBridge TypeScript Definitions
 * Auto-generated by generate-registry.js
 * 
 * Usage:
 *   const bridge: UniversalBridge = require('./bridge');
 *   bridge.add(5, 3);  // Full autocomplete!
 */

export interface UniversalBridge {
    // Core functions
    reload(): UniversalBridge;
    _version: string;
    _getInfo(): { version: string; dllPath: string; functions: string[] };
    
    // Magic Store
    Store: {
        get(key: string): string;
        set(key: string, value: string): void;
        getInt(key: string): number;
        setInt(key: string, value: number): void;
        dump(): Record<string, any>;
    };
    
    // Enterprise: Callback Registration
    registerCallback(name: string, fn: (...args: any[]) => void): void;
    
    // Enterprise: Class Factory (Auto-Cleanup!)
    createClass(className: string): {
        create(): { handle: number; className: string };
        delete(wrapper: { handle: number }): void;
    } | null;
    
    // =========================================================================
    // Generated C++ Functions
    // =========================================================================
`;

    for (const fn of functions) {
        const tsReturn = mapToTS(fn.returnType);
        const tsParams = fn.params.map((p, i) => {
            const paramName = p.name || `arg${i}`;
            const paramType = mapToTS(p.type);
            return `${paramName}: ${paramType}`;
        }).join(', ');

        ts += `    ${fn.name}(${tsParams}): ${tsReturn};\n`;
    }

    ts += `}

declare const bridge: UniversalBridge;
export default bridge;
`;

    return ts;
}

// =============================================================================
// MAIN
// =============================================================================

function main() {
    console.log('╔══════════════════════════════════════════════════════════╗');
    console.log('║  Secure Registry & TypeScript Generator v6.0             ║');
    console.log('╚══════════════════════════════════════════════════════════╝\n');

    console.log('🔒 Security: All protections enabled\n');
    console.log(`📂 Scanning: ${HUB_DIR}\n`);

    if (!isPathSafe(HUB_DIR)) {
        console.error('❌ Security: Invalid scan directory');
        process.exit(1);
    }

    const functions = scanDirectory(HUB_DIR);

    // Remove duplicates
    const seen = new Set();
    const unique = functions.filter(f => {
        if (seen.has(f.name)) return false;
        seen.add(f.name);
        return true;
    });

    // Load existing registry
    let registry = { version: '6.0.0', functions: {} };
    if (fs.existsSync(REGISTRY_PATH)) {
        try {
            registry = JSON.parse(fs.readFileSync(REGISTRY_PATH, 'utf8'));
        } catch (e) { }
    }

    // Update version
    registry.version = '6.0.0';

    // Merge new functions
    let added = 0;
    for (const fn of unique) {
        if (!registry.functions[fn.name]) {
            registry.functions[fn.name] = {
                returnType: fn.returnType,
                params: fn.params.map(p => p.type),
                async: fn.async
            };
            added++;
            console.log(`  + ${fn.name}`);
        }
    }

    // Save registry
    fs.writeFileSync(REGISTRY_PATH, JSON.stringify(registry, null, 2));

    // Generate TypeScript
    const allFns = Object.entries(registry.functions).map(([name, info]) => ({
        name,
        returnType: info.returnType,
        params: info.params.map((t, i) => ({ type: t, name: `arg${i}` })),
        async: info.async
    }));

    const tsContent = generateTypeScript(allFns);
    fs.writeFileSync(TYPES_PATH, tsContent);

    console.log(`\n✅ Registry updated: ${Object.keys(registry.functions).length} functions`);
    console.log(`✅ TypeScript types: bridge.d.ts generated`);
    console.log(`   New functions added: ${added}`);
}

main();
