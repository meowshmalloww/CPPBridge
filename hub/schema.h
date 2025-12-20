#pragma once
#include <stdint.h>
#include <vector>
#include <string>

#ifdef GENERATOR_RUNNING 
    #define BRIDGE_STRUCT __attribute__((annotate("bridge::struct")))
    #define BRIDGE_FIELD __attribute__((annotate("bridge::field")))
#else
    #define BRIDGE_STRUCT 
    #define BRIDGE_FIELD 
#endif

namespace Hub {

    struct Handle {
        uint32_t index;
    };

    // 1. DATA CONTAINERS (Big Data / Files)
    struct BRIDGE_STRUCT BigData {
        BRIDGE_FIELD std::vector<int> allIds;
        BRIDGE_FIELD std::vector<int> allScores;
        BRIDGE_FIELD std::vector<uint8_t> binaryBlob;
    };

    // 2. DATABASE CONFIG (SQL)
    struct BRIDGE_STRUCT SQLConfig {
        BRIDGE_FIELD std::string query;
        BRIDGE_FIELD std::string connectionString;
    };

    // 3. SERVER/API CONFIG (Backend Logic)
    struct BRIDGE_STRUCT ServerRequest {
        BRIDGE_FIELD std::string url;
        BRIDGE_FIELD std::string method;
        BRIDGE_FIELD std::string payload;
    };

    struct BRIDGE_STRUCT ServerResponse {
        BRIDGE_FIELD int32_t statusCode;
        BRIDGE_FIELD std::string body;
    };
    
    // 4. CAMERA FRAME (Restored for compatibility)
    struct BRIDGE_STRUCT CameraFrame {
        BRIDGE_FIELD int32_t width;
        BRIDGE_FIELD int32_t height;
        BRIDGE_FIELD std::vector<uint8_t> pixelBuffer; // <--- Restored!
    };
}