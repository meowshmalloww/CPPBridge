cmake_minimum_required(VERSION 3.15)
project(UniversalBridge VERSION 2.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# SQLITE3 LIBRARY (Compiled as C)
# =============================================================================
add_library(sqlite3 STATIC
    hub/modules/database/sqlite3.c
)
target_include_directories(sqlite3 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/hub/modules/database
)
# Disable warnings for third-party code
if(MSVC)
    target_compile_options(sqlite3 PRIVATE /W0)
endif()

# =============================================================================
# GATHER ALL SOURCES
# =============================================================================
file(GLOB_RECURSE CORE_SOURCES 
    "hub/core/*.cpp"
    "hub/core/*.h"
)

file(GLOB_RECURSE MODULE_SOURCES 
    "hub/modules/**/*.cpp"
    "hub/modules/**/*.h"
)
# Exclude sqlite3.c from C++ sources (it's compiled separately)
list(FILTER MODULE_SOURCES EXCLUDE REGEX ".*sqlite3\\.c$")
# Exclude biometric module (deleted/empty stubs)
list(FILTER MODULE_SOURCES EXCLUDE REGEX ".*biometric\\.(cpp|h)$")

set(HUB_SOURCES
    hub/schema.h
    hub/arena.h
    hub/hub.cpp
    hub/generated_bridge.cpp
    hub/bridge_runtime.cpp
    hub/example_bridge.cpp
)

# =============================================================================
# MAIN DLL LIBRARY
# =============================================================================
add_library(UniversalBridge SHARED 
    ${HUB_SOURCES}
    ${CORE_SOURCES}
    ${MODULE_SOURCES}
)

target_include_directories(UniversalBridge PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/hub
    ${CMAKE_CURRENT_SOURCE_DIR}/hub/core
    ${CMAKE_CURRENT_SOURCE_DIR}/hub/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/hub/modules/database
    ${CMAKE_CURRENT_SOURCE_DIR}/hub/jsi
)

# =============================================================================
# PLATFORM-SPECIFIC LIBRARIES
# =============================================================================
if(WIN32)
    target_link_libraries(UniversalBridge PRIVATE wininet sqlite3)
else()
    # Linux/macOS: use libcurl for HTTP, pthread for threading
    find_package(CURL REQUIRED)
    find_package(Threads REQUIRED)
    target_link_libraries(UniversalBridge PRIVATE 
        CURL::libcurl 
        Threads::Threads 
        sqlite3
    )
    target_compile_definitions(UniversalBridge PRIVATE HUB_PLATFORM_POSIX)
endif()

# =============================================================================
# TEST EXECUTABLE (Disabled - main.cpp not present)
# =============================================================================
# add_executable(BridgeTest 
#     main.cpp 
#     hub/hub.cpp
#     ${CORE_SOURCES}
#     ${MODULE_SOURCES}
# )

# target_include_directories(BridgeTest PUBLIC 
#     ${CMAKE_CURRENT_SOURCE_DIR}/hub
#     ${CMAKE_CURRENT_SOURCE_DIR}/hub/core
#     ${CMAKE_CURRENT_SOURCE_DIR}/hub/modules
#     ${CMAKE_CURRENT_SOURCE_DIR}/hub/modules/database
#     ${CMAKE_CURRENT_SOURCE_DIR}/hub/jsi
# )

# =============================================================================
# UNIT TESTS
# =============================================================================
add_executable(UnitTests 
    tests/test_all.cpp
    ${CORE_SOURCES}
    ${MODULE_SOURCES}
)

target_include_directories(UnitTests PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/hub
    ${CMAKE_CURRENT_SOURCE_DIR}/hub/core
    ${CMAKE_CURRENT_SOURCE_DIR}/hub/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/hub/modules/database
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

if(WIN32)
    # target_link_libraries(BridgeTest PRIVATE wininet sqlite3)
    target_link_libraries(UnitTests PRIVATE wininet sqlite3)
else()
    find_package(CURL REQUIRED)
    find_package(Threads REQUIRED)
    # target_link_libraries(BridgeTest PRIVATE CURL::libcurl Threads::Threads sqlite3)
    target_link_libraries(UnitTests PRIVATE CURL::libcurl Threads::Threads sqlite3)
endif()

# =============================================================================
# COMPILER OPTIONS
# =============================================================================
if(MSVC)
    target_compile_options(UniversalBridge PRIVATE /W4 /permissive-)
    # target_compile_options(BridgeTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(UniversalBridge PRIVATE -Wall -Wextra -pedantic)
    # target_compile_options(BridgeTest PRIVATE -Wall -Wextra -pedantic)
endif()

message(STATUS "UniversalBridge v${PROJECT_VERSION}")
message(STATUS "Core sources: ${CORE_SOURCES}")
message(STATUS "Module sources: ${MODULE_SOURCES}")

