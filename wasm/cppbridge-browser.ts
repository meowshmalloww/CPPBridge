/**
 * CPPBridge WebAssembly Module for Browsers
 * 
 * Usage:
 *   <script src="cppbridge.js"></script>
 *   <script>
 *     CPPBridge().then(bridge => {
 *       const crypto = new bridge.Crypto();
 *       console.log(crypto.sha256('hello'));
 *     });
 *   </script>
 */

// Type definitions (used when cppbridge module is built)
interface Database {
    open(path: string): number;
    exec(handle: number, sql: string): boolean;
    query(handle: number, sql: string): string;
    close(handle: number): void;
    delete(): void;
}

interface KeyValue {
    open(path: string): number;
    set(handle: number, key: string, value: string): boolean;
    get(handle: number, key: string): string;
    remove(handle: number, key: string): boolean;
    close(handle: number): void;
    delete(): void;
}

interface Crypto {
    sha256(data: string): string;
    base64Encode(data: string): string;
    base64Decode(data: string): string;
    delete(): void;
}

interface Compression {
    compress(data: string): Uint8Array;
    decompress(data: Uint8Array): string;
    delete(): void;
}

interface CPPBridgeModule {
    Database: new () => Database;
    KeyValue: new () => KeyValue;
    Crypto: new () => Crypto;
    Compression: new () => Compression;
}

// Declare the global CPPBridge function (loaded from cppbridge.js)
declare function CPPBridge(): Promise<CPPBridgeModule>;

// Convenience wrapper for easier usage
const loadCPPBridge = async () => {
    const module = await CPPBridge();

    return {
        // Crypto utilities
        crypto: {
            sha256: (data: string) => {
                const c = new module.Crypto();
                const result = c.sha256(data);
                c.delete();
                return result;
            },
            base64Encode: (data: string) => {
                const c = new module.Crypto();
                const result = c.base64Encode(data);
                c.delete();
                return result;
            },
            base64Decode: (data: string) => {
                const c = new module.Crypto();
                const result = c.base64Decode(data);
                c.delete();
                return result;
            },
        },

        // Database (persistent in IndexedDB via Emscripten FS)
        database: {
            open: (path: string) => {
                const db = new module.Database();
                const handle = db.open(path);
                return { handle, db };
            },
            exec: (db: { handle: number; db: Database }, sql: string) => db.db.exec(db.handle, sql),
            query: (db: { handle: number; db: Database }, sql: string) => JSON.parse(db.db.query(db.handle, sql)),
            close: (db: { handle: number; db: Database }) => {
                db.db.close(db.handle);
                db.db.delete();
            },
        },

        // Key-Value store
        keyvalue: {
            open: (path: string) => {
                const kv = new module.KeyValue();
                const handle = kv.open(path);
                return { handle, kv };
            },
            set: (store: { handle: number; kv: KeyValue }, key: string, value: string) => store.kv.set(store.handle, key, value),
            get: (store: { handle: number; kv: KeyValue }, key: string) => store.kv.get(store.handle, key),
            delete: (store: { handle: number; kv: KeyValue }, key: string) => store.kv.remove(store.handle, key),
            close: (store: { handle: number; kv: KeyValue }) => {
                store.kv.close(store.handle);
                store.kv.delete();
            },
        },

        // Raw module access
        _module: module,
    };
};

// Export for ES modules
export { loadCPPBridge };
export type { Database, KeyValue, Crypto, Compression, CPPBridgeModule };
