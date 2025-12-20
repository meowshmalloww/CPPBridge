import ctypes
import os

print("--- REAL INTERNET TEST (C++) ---")
dll_path = os.path.abspath("build/UniversalBridge.dll")
if not os.path.exists(dll_path): dll_path = os.path.abspath("build/Release/UniversalBridge.dll")

lib = ctypes.CDLL(dll_path)

# 1. SETUP TYPES
lib.create_server_request.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
lib.create_server_request.restype = ctypes.c_int

lib.send_server_request.argtypes = [ctypes.c_int]
lib.send_server_request.restype = ctypes.c_int

# We will use the auto-generated getter to check the body size
lib.ServerResponse_get_body_size.argtypes = [ctypes.c_int]
lib.ServerResponse_get_body_size.restype = ctypes.c_int

# 2. DEFINE REQUEST
target_url = b"https://www.google.com"
print(f"\n[App] Asking C++ to fetch: {target_url.decode()}")

# 3. EXECUTE
req_ticket = lib.create_server_request(target_url, b"GET")
resp_ticket = lib.send_server_request(req_ticket)

# 4. VERIFY RESULT
# If we downloaded HTML, the body size will be large (e.g. > 10,000 bytes)
size = lib.ServerResponse_get_body_size(resp_ticket)

if size > 1000:
    print(f"✅ SUCCESS: C++ downloaded {size} bytes of real HTML data.")
    print("   Your Universal Bridge is now online.")
elif size == 0:
    print("❌ FAILED: C++ returned 0 bytes. Check your internet connection.")
else:
    print(f"⚠️  WARNING: Data seems small ({size} bytes). But connection worked.")

print("--- TEST COMPLETE ---")