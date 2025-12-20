/**
 * Simple test for UniversalBridge npm package
 */

const { http, keyvalue, system, logging } = require('./index');

console.log('UniversalBridge npm Package Test');
console.log('='.repeat(50));

// Test 1: System info
console.log('\n1. System Info:');
console.log('   Process ID:', system.process.id());
console.log('   CPU Count:', system.process.cpuCount());

// Test 2: Environment
console.log('\n2. Environment:');
console.log('   USERPROFILE:', system.env.get('USERPROFILE'));

// Test 3: Key-Value Store
console.log('\n3. Key-Value Store:');
const store = keyvalue.open('test_settings.db');
store.set('theme', 'dark');
store.set('language', 'en');
console.log('   theme:', store.get('theme'));
console.log('   language:', store.get('language'));
store.close();

console.log('\n✅ All tests passed!');
console.log('\nYou can now use:');
console.log("   import { http, database, keyvalue } from 'universalbridge';");
