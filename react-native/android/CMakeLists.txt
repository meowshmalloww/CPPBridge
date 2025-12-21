cmake_minimum_required(VERSION 3.18.1)
project(cppbridge)

set(CMAKE_CXX_STANDARD 17)

# Find all CPPBridge source files
file(GLOB_RECURSE CPPBRIDGE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../hub/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../hub/modules/**/*.cpp"
)

# Create shared library
add_library(cppbridge SHARED
    ${CPPBRIDGE_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/jni-bridge.cpp
)

target_include_directories(cppbridge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../hub
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../hub/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../hub/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../hub/modules/database
)

# Android NDK log library
find_library(log-lib log)

target_link_libraries(cppbridge
    ${log-lib}
    android
)
