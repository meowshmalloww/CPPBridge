cmake_minimum_required(VERSION 3.15)
project(cppbridge-wasm)

set(CMAKE_CXX_STANDARD 17)

# Emscripten settings for WebAssembly
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    
    # Gather sources (exclude Windows-specific files)
    file(GLOB_RECURSE CORE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../hub/core/*.cpp"
    )
    
    file(GLOB_RECURSE MODULE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../hub/modules/database/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../hub/modules/security/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../hub/modules/utils/*.cpp"
    )
    
    # Exclude Windows-specific files
    list(FILTER CORE_SOURCES EXCLUDE REGEX ".*crashhandler.*")
    list(FILTER MODULE_SOURCES EXCLUDE REGEX ".*clipboard.*")
    list(FILTER MODULE_SOURCES EXCLUDE REGEX ".*sysinfo.*")
    list(FILTER MODULE_SOURCES EXCLUDE REGEX ".*sysutils.*")
    
    # Create WASM module
    add_executable(cppbridge
        ${CMAKE_CURRENT_SOURCE_DIR}/wasm-bindings.cpp
        ${CORE_SOURCES}
        ${MODULE_SOURCES}
    )
    
    target_include_directories(cppbridge PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../hub
        ${CMAKE_CURRENT_SOURCE_DIR}/../hub/core
        ${CMAKE_CURRENT_SOURCE_DIR}/../hub/modules
        ${CMAKE_CURRENT_SOURCE_DIR}/../hub/modules/database
    )
    
    # Emscripten linker flags
    set_target_properties(cppbridge PROPERTIES
        LINK_FLAGS "-s WASM=1 \
                    -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8'] \
                    -s ALLOW_MEMORY_GROWTH=1 \
                    -s MODULARIZE=1 \
                    -s EXPORT_NAME='CPPBridge' \
                    -s ENVIRONMENT='web,worker' \
                    -s FILESYSTEM=1 \
                    -s EXPORTED_FUNCTIONS='[\"_malloc\",\"_free\"]' \
                    --bind"
    )
    
endif()
