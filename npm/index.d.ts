/**
 * TypeScript definitions for UniversalBridge
 */

export interface HttpResponse {
    status: number;
    body: string;
}

export interface HttpModule {
    get(url: string, retries?: number): HttpResponse;
    post(url: string, body: string, contentType?: string, retries?: number): HttpResponse;
}

export interface Database {
    open(path: string): boolean;
    exec(sql: string): boolean;
    query(sql: string): any;
    close(): void;
}

export interface DatabaseModule {
    Database: new () => Database;
    open(path: string): Database;
}

export interface KVStore {
    open(path: string): boolean;
    set(key: string, value: string): boolean;
    get(key: string): string | null;
    delete(key: string): boolean;
    close(): void;
}

export interface KeyValueModule {
    KVStore: new () => KVStore;
    open(path: string): KVStore;
}

export interface SystemModule {
    env: {
        get(name: string): string | null;
        set(name: string, value: string): boolean;
    };
    process: {
        id(): number;
        cpuCount(): number;
        spawn(command: string, workDir?: string, hidden?: boolean): number;
        kill(pid: number): boolean;
    };
}

export interface LoggingModule {
    LogLevel: {
        TRACE: 0;
        DEBUG: 1;
        INFO: 2;
        WARN: 3;
        ERROR: 4;
        FATAL: 5;
    };
    init(logDir: string, prefix?: string, maxSizeMb?: number, maxFiles?: number): void;
    log(level: number, message: string, source?: string): void;
    info(message: string, source?: string): void;
    warn(message: string, source?: string): void;
    error(message: string, source?: string): void;
    shutdown(): void;
}

export const http: HttpModule;
export const database: DatabaseModule;
export const keyvalue: KeyValueModule;
export const system: SystemModule;
export const logging: LoggingModule;
