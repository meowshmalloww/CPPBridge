// @ts-ignore - Types come from @types/react-native when installed
import { NativeModules, Platform } from 'react-native';

const LINKING_ERROR =
    `The package 'react-native-cppbridge' doesn't seem to be linked. Make sure: \n\n` +
    Platform.select({ ios: "- You have run 'pod install'\n", default: '' }) +
    '- You rebuilt the app after installing the package\n' +
    '- You are not using Expo Go (use development build instead)\n';

const CppBridge = NativeModules.CppBridge
    ? NativeModules.CppBridge
    : new Proxy(
        {},
        {
            get() {
                throw new Error(LINKING_ERROR);
            },
        }
    );

// =============================================================================
// HTTP Module
// =============================================================================
export const http = {
    async get(url: string): Promise<{ status: number; body: string }> {
        return CppBridge.httpGet(url);
    },

    async post(
        url: string,
        body: string,
        contentType: string = 'application/json'
    ): Promise<{ status: number; body: string }> {
        return CppBridge.httpPost(url, body, contentType);
    },
};

// =============================================================================
// Database Module
// =============================================================================
export const database = {
    async open(path: string): Promise<number> {
        return CppBridge.dbOpen(path);
    },

    async exec(handle: number, sql: string): Promise<boolean> {
        return CppBridge.dbExec(handle, sql);
    },

    async query(handle: number, sql: string): Promise<any[]> {
        const result = await CppBridge.dbQuery(handle, sql);
        return JSON.parse(result);
    },

    async close(handle: number): Promise<void> {
        return CppBridge.dbClose(handle);
    },
};

// =============================================================================
// Key-Value Store
// =============================================================================
export const keyvalue = {
    async open(path: string): Promise<number> {
        return CppBridge.kvOpen(path);
    },

    async set(handle: number, key: string, value: string): Promise<boolean> {
        return CppBridge.kvSet(handle, key, value);
    },

    async get(handle: number, key: string): Promise<string | null> {
        return CppBridge.kvGet(handle, key);
    },

    async delete(handle: number, key: string): Promise<boolean> {
        return CppBridge.kvDelete(handle, key);
    },

    async close(handle: number): Promise<void> {
        return CppBridge.kvClose(handle);
    },
};

// =============================================================================
// Crypto Module
// =============================================================================
export const crypto = {
    async sha256(data: string): Promise<string> {
        return CppBridge.cryptoSha256(data);
    },

    async aesEncrypt(data: string, key: string): Promise<string> {
        return CppBridge.cryptoAesEncrypt(data, key);
    },

    async aesDecrypt(data: string, key: string): Promise<string> {
        return CppBridge.cryptoAesDecrypt(data, key);
    },
};

// =============================================================================
// System Module
// =============================================================================
export const system = {
    async cpuCount(): Promise<number> {
        return CppBridge.systemCpuCount();
    },

    async processId(): Promise<number> {
        return CppBridge.systemProcessId();
    },
};

// Default export
export default {
    http,
    database,
    keyvalue,
    crypto,
    system,
};
