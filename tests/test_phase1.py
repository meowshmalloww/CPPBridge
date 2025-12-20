"""
UniversalBridge Phase 1 Test Suite
Tests core functionality: HTTP, FileSystem, Async, Error Handling
"""
import ctypes
import os
import time
import tempfile
import json

print("=" * 60)
print("  UNIVERSAL BRIDGE - PHASE 1 TEST SUITE")
print("=" * 60)

# Load DLL
dll_path = os.path.abspath("build/UniversalBridge.dll")
if not os.path.exists(dll_path):
    dll_path = os.path.abspath("build/Release/UniversalBridge.dll")
if not os.path.exists(dll_path):
    dll_path = os.path.abspath("build/Debug/UniversalBridge.dll")

if not os.path.exists(dll_path):
    print(f"❌ ERROR: DLL not found. Run cmake build first.")
    print(f"   Expected: {dll_path}")
    exit(1)

print(f"📦 Loading DLL: {dll_path}")
lib = ctypes.CDLL(dll_path)

# ============================================================================
# SETUP FUNCTION SIGNATURES
# ============================================================================

# System Lifecycle
lib.hub_init.argtypes = []
lib.hub_init.restype = None

lib.hub_shutdown.argtypes = []
lib.hub_shutdown.restype = None

lib.hub_version.argtypes = []
lib.hub_version.restype = ctypes.c_char_p

# Legacy HTTP API
lib.create_server_request.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
lib.create_server_request.restype = ctypes.c_int

lib.send_server_request.argtypes = [ctypes.c_int]
lib.send_server_request.restype = ctypes.c_int

lib.ServerResponse_get_body_size.argtypes = [ctypes.c_int]
lib.ServerResponse_get_body_size.restype = ctypes.c_int

lib.release_handle.argtypes = [ctypes.c_int]
lib.release_handle.restype = None

# New HTTP API
lib.hub_http_get.argtypes = [ctypes.c_char_p]
lib.hub_http_get.restype = ctypes.c_int

lib.hub_http_response_status.argtypes = [ctypes.c_int]
lib.hub_http_response_status.restype = ctypes.c_int

lib.hub_http_response_body_size.argtypes = [ctypes.c_int]
lib.hub_http_response_body_size.restype = ctypes.c_int

lib.hub_http_response_release.argtypes = [ctypes.c_int]
lib.hub_http_response_release.restype = None

# File System API
lib.hub_file_write.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
lib.hub_file_write.restype = ctypes.c_int

lib.hub_file_exists.argtypes = [ctypes.c_char_p]
lib.hub_file_exists.restype = ctypes.c_int

lib.hub_file_delete.argtypes = [ctypes.c_char_p]
lib.hub_file_delete.restype = ctypes.c_int

lib.hub_file_copy.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
lib.hub_file_copy.restype = ctypes.c_int

lib.hub_dir_create.argtypes = [ctypes.c_char_p]
lib.hub_dir_create.restype = ctypes.c_int

lib.hub_file_get_error.argtypes = []
lib.hub_file_get_error.restype = ctypes.c_char_p

# Async API
lib.hub_async_init.argtypes = [ctypes.c_int]
lib.hub_async_init.restype = None

lib.hub_async_pending_count.argtypes = []
lib.hub_async_pending_count.restype = ctypes.c_int

# ============================================================================
# TESTS
# ============================================================================
tests_passed = 0
tests_failed = 0

def test(name, condition, details=""):
    global tests_passed, tests_failed
    if condition:
        print(f"  ✅ {name}")
        tests_passed += 1
    else:
        print(f"  ❌ {name}")
        if details:
            print(f"     ↳ {details}")
        tests_failed += 1

# ----------------------------------------------------------------------------
print("\n🔧 TEST 1: System Initialization")
# ----------------------------------------------------------------------------
try:
    lib.hub_init()
    version = lib.hub_version().decode('utf-8')
    test("hub_init() succeeds", True)
    test(f"Version is 2.0.0", version == "2.0.0", f"Got: {version}")
except Exception as e:
    test("Initialization", False, str(e))

# ----------------------------------------------------------------------------
print("\n🌐 TEST 2: Legacy HTTP API (Backward Compatibility)")
# ----------------------------------------------------------------------------
try:
    target_url = b"https://www.google.com"
    req_handle = lib.create_server_request(target_url, b"GET")
    test("create_server_request() returns handle", req_handle >= 0, f"Handle: {req_handle}")
    
    resp_handle = lib.send_server_request(req_handle)
    test("send_server_request() returns handle", resp_handle >= 0, f"Handle: {resp_handle}")
    
    body_size = lib.ServerResponse_get_body_size(resp_handle)
    test("Response body size > 1000 bytes", body_size > 1000, f"Size: {body_size}")
    
    lib.release_handle(resp_handle)
    test("release_handle() succeeds", True)
except Exception as e:
    test("Legacy HTTP", False, str(e))

# ----------------------------------------------------------------------------
print("\n🌐 TEST 3: New HTTP Client API")
# ----------------------------------------------------------------------------
try:
    resp_handle = lib.hub_http_get(b"https://httpbin.org/get")
    test("hub_http_get() returns handle", resp_handle >= 0, f"Handle: {resp_handle}")
    
    status = lib.hub_http_response_status(resp_handle)
    test("HTTP status is 200", status == 200, f"Status: {status}")
    
    size = lib.hub_http_response_body_size(resp_handle)
    test("Response body exists", size > 0, f"Size: {size}")
    
    lib.hub_http_response_release(resp_handle)
    test("hub_http_response_release() succeeds", True)
except Exception as e:
    test("New HTTP Client", False, str(e))

# ----------------------------------------------------------------------------
print("\n📁 TEST 4: File System Operations")
# ----------------------------------------------------------------------------
try:
    # Create temp directory for tests
    test_dir = os.path.join(tempfile.gettempdir(), "UniversalBridge_Test")
    test_file = os.path.join(test_dir, "test_file.txt")
    test_copy = os.path.join(test_dir, "test_copy.txt")
    
    # Create directory
    result = lib.hub_dir_create(test_dir.encode('utf-8'))
    test("hub_dir_create() succeeds", result == 0)
    
    # Write file
    content = b"Hello from UniversalBridge v2.0!"
    result = lib.hub_file_write(test_file.encode('utf-8'), content)
    test("hub_file_write() succeeds", result == 0)
    
    # Check exists
    exists = lib.hub_file_exists(test_file.encode('utf-8'))
    test("hub_file_exists() returns true", exists == 1)
    
    # Verify content
    with open(test_file, 'r') as f:
        read_content = f.read()
    test("File content matches", read_content == content.decode(), f"Got: {read_content[:50]}...")
    
    # Copy file
    result = lib.hub_file_copy(test_file.encode('utf-8'), test_copy.encode('utf-8'), 0)
    test("hub_file_copy() succeeds", result == 0)
    
    copy_exists = lib.hub_file_exists(test_copy.encode('utf-8'))
    test("Copied file exists", copy_exists == 1)
    
    # Delete files
    result = lib.hub_file_delete(test_file.encode('utf-8'))
    test("hub_file_delete() succeeds", result == 0)
    
    deleted = lib.hub_file_exists(test_file.encode('utf-8'))
    test("Deleted file no longer exists", deleted == 0)
    
    # Cleanup
    lib.hub_file_delete(test_copy.encode('utf-8'))
    os.rmdir(test_dir)
except Exception as e:
    test("File System", False, str(e))

# ----------------------------------------------------------------------------
print("\n⚡ TEST 5: Async System")
# ----------------------------------------------------------------------------
try:
    pending = lib.hub_async_pending_count()
    test("hub_async_pending_count() works", pending >= 0, f"Pending: {pending}")
except Exception as e:
    test("Async System", False, str(e))

# ----------------------------------------------------------------------------
print("\n🔌 TEST 6: Shutdown")
# ----------------------------------------------------------------------------
try:
    lib.hub_shutdown()
    test("hub_shutdown() succeeds", True)
except Exception as e:
    test("Shutdown", False, str(e))

# ============================================================================
# SUMMARY
# ============================================================================
print("\n" + "=" * 60)
total = tests_passed + tests_failed
print(f"  RESULTS: {tests_passed}/{total} tests passed")
if tests_failed == 0:
    print("  🎉 ALL TESTS PASSED!")
else:
    print(f"  ⚠️  {tests_failed} test(s) failed")
print("=" * 60)
