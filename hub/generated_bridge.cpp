
#pragma once
#include "../hub/schema.h"
#include "../hub/arena.h"
#include <jsi/jsi.h>
#include <vector>
#include <string>
#include <cstring> 

using namespace facebook;

namespace Bridge {
    
    class JSI_EXPORT BigDataHostObject : public jsi::HostObject {
        Hub::Handle handle;
    public:
        BigDataHostObject(Hub::Handle h) : handle(h) {}

        jsi::Value get(jsi::Runtime& rt, const jsi::PropNameID& name) override {
            std::string propName = name.utf8(rt);
            // SMART ARENA LOOKUP
            auto* basePtr = Hub::bigDataArena.access(this->handle);
            if (!basePtr) return jsi::Value::undefined();

            
            if (propName == "allIds") {
                auto* vecPtr = reinterpret_cast<std::vector<int>*>( (uint8_t*)basePtr + 0 );
                jsi::Array arr = jsi::Array(rt, vecPtr->size());
                for (size_t i = 0; i < vecPtr->size(); ++i) arr.setValueAtIndex(rt, i, (double)(*vecPtr)[i]);
                return arr;
            }
            if (propName == "allScores") {
                auto* vecPtr = reinterpret_cast<std::vector<int>*>( (uint8_t*)basePtr + 24 );
                jsi::Array arr = jsi::Array(rt, vecPtr->size());
                for (size_t i = 0; i < vecPtr->size(); ++i) arr.setValueAtIndex(rt, i, (double)(*vecPtr)[i]);
                return arr;
            }
            if (propName == "binaryBlob") {
                auto* vecPtr = reinterpret_cast<std::vector<uint8_t>*>( (uint8_t*)basePtr + 48 );
                jsi::Array arr = jsi::Array(rt, vecPtr->size());
                for (size_t i = 0; i < vecPtr->size(); ++i) arr.setValueAtIndex(rt, i, (double)(*vecPtr)[i]);
                return arr;
            }

            return jsi::Value::undefined();
        }
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override {
            std::vector<jsi::PropNameID> names;
            names.push_back(jsi::PropNameID::forUtf8(rt, "allIds"));
names.push_back(jsi::PropNameID::forUtf8(rt, "allScores"));
names.push_back(jsi::PropNameID::forUtf8(rt, "binaryBlob"));

            return names;
        }
    };

    class JSI_EXPORT SQLConfigHostObject : public jsi::HostObject {
        Hub::Handle handle;
    public:
        SQLConfigHostObject(Hub::Handle h) : handle(h) {}

        jsi::Value get(jsi::Runtime& rt, const jsi::PropNameID& name) override {
            std::string propName = name.utf8(rt);
            // SMART ARENA LOOKUP
            auto* basePtr = Hub::sqlArena.access(this->handle);
            if (!basePtr) return jsi::Value::undefined();

            
            if (propName == "query") {
                auto* strPtr = reinterpret_cast<std::string*>( (uint8_t*)basePtr + 0 );
                return jsi::String::createFromUtf8(rt, *strPtr);
            }
            if (propName == "connectionString") {
                auto* strPtr = reinterpret_cast<std::string*>( (uint8_t*)basePtr + 32 );
                return jsi::String::createFromUtf8(rt, *strPtr);
            }

            return jsi::Value::undefined();
        }
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override {
            std::vector<jsi::PropNameID> names;
            names.push_back(jsi::PropNameID::forUtf8(rt, "query"));
names.push_back(jsi::PropNameID::forUtf8(rt, "connectionString"));

            return names;
        }
    };

    class JSI_EXPORT ServerRequestHostObject : public jsi::HostObject {
        Hub::Handle handle;
    public:
        ServerRequestHostObject(Hub::Handle h) : handle(h) {}

        jsi::Value get(jsi::Runtime& rt, const jsi::PropNameID& name) override {
            std::string propName = name.utf8(rt);
            // SMART ARENA LOOKUP
            auto* basePtr = Hub::requestArena.access(this->handle);
            if (!basePtr) return jsi::Value::undefined();

            
            if (propName == "url") {
                auto* strPtr = reinterpret_cast<std::string*>( (uint8_t*)basePtr + 0 );
                return jsi::String::createFromUtf8(rt, *strPtr);
            }
            if (propName == "method") {
                auto* strPtr = reinterpret_cast<std::string*>( (uint8_t*)basePtr + 32 );
                return jsi::String::createFromUtf8(rt, *strPtr);
            }
            if (propName == "payload") {
                auto* strPtr = reinterpret_cast<std::string*>( (uint8_t*)basePtr + 64 );
                return jsi::String::createFromUtf8(rt, *strPtr);
            }

            return jsi::Value::undefined();
        }
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override {
            std::vector<jsi::PropNameID> names;
            names.push_back(jsi::PropNameID::forUtf8(rt, "url"));
names.push_back(jsi::PropNameID::forUtf8(rt, "method"));
names.push_back(jsi::PropNameID::forUtf8(rt, "payload"));

            return names;
        }
    };

    class JSI_EXPORT ServerResponseHostObject : public jsi::HostObject {
        Hub::Handle handle;
    public:
        ServerResponseHostObject(Hub::Handle h) : handle(h) {}

        jsi::Value get(jsi::Runtime& rt, const jsi::PropNameID& name) override {
            std::string propName = name.utf8(rt);
            // SMART ARENA LOOKUP
            auto* basePtr = Hub::responseArena.access(this->handle);
            if (!basePtr) return jsi::Value::undefined();

            
            if (propName == "statusCode") {
                auto* fieldPtr = reinterpret_cast<int32_t*>( (uint8_t*)basePtr + 0 );
                return jsi::Value((double)(*fieldPtr));
            }
            if (propName == "body") {
                auto* strPtr = reinterpret_cast<std::string*>( (uint8_t*)basePtr + 8 );
                return jsi::String::createFromUtf8(rt, *strPtr);
            }

            return jsi::Value::undefined();
        }
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override {
            std::vector<jsi::PropNameID> names;
            names.push_back(jsi::PropNameID::forUtf8(rt, "statusCode"));
names.push_back(jsi::PropNameID::forUtf8(rt, "body"));

            return names;
        }
    };

    class JSI_EXPORT CameraFrameHostObject : public jsi::HostObject {
        Hub::Handle handle;
    public:
        CameraFrameHostObject(Hub::Handle h) : handle(h) {}

        jsi::Value get(jsi::Runtime& rt, const jsi::PropNameID& name) override {
            std::string propName = name.utf8(rt);
            // SMART ARENA LOOKUP
            auto* basePtr = Hub::frameArena.access(this->handle);
            if (!basePtr) return jsi::Value::undefined();

            
            if (propName == "width") {
                auto* fieldPtr = reinterpret_cast<int32_t*>( (uint8_t*)basePtr + 0 );
                return jsi::Value((double)(*fieldPtr));
            }
            if (propName == "height") {
                auto* fieldPtr = reinterpret_cast<int32_t*>( (uint8_t*)basePtr + 4 );
                return jsi::Value((double)(*fieldPtr));
            }
            if (propName == "pixelBuffer") {
                auto* vecPtr = reinterpret_cast<std::vector<uint8_t>*>( (uint8_t*)basePtr + 8 );
                jsi::Array arr = jsi::Array(rt, vecPtr->size());
                for (size_t i = 0; i < vecPtr->size(); ++i) arr.setValueAtIndex(rt, i, (double)(*vecPtr)[i]);
                return arr;
            }

            return jsi::Value::undefined();
        }
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override {
            std::vector<jsi::PropNameID> names;
            names.push_back(jsi::PropNameID::forUtf8(rt, "width"));
names.push_back(jsi::PropNameID::forUtf8(rt, "height"));
names.push_back(jsi::PropNameID::forUtf8(rt, "pixelBuffer"));

            return names;
        }
    };

}

extern "C" {
    
    __declspec(dllexport) int BigData_get_allIds_size(int handleIndex) {
        auto* ptr = Hub::bigDataArena.access({ (uint32_t)handleIndex });
        if (!ptr) return 0;
        auto* vecPtr = reinterpret_cast<std::vector<int>*>( (uint8_t*)ptr + 0 );
        return (int)vecPtr->size();
    }

    __declspec(dllexport) int BigData_get_allScores_size(int handleIndex) {
        auto* ptr = Hub::bigDataArena.access({ (uint32_t)handleIndex });
        if (!ptr) return 0;
        auto* vecPtr = reinterpret_cast<std::vector<int>*>( (uint8_t*)ptr + 24 );
        return (int)vecPtr->size();
    }

    __declspec(dllexport) int BigData_get_binaryBlob_size(int handleIndex) {
        auto* ptr = Hub::bigDataArena.access({ (uint32_t)handleIndex });
        if (!ptr) return 0;
        auto* vecPtr = reinterpret_cast<std::vector<uint8_t>*>( (uint8_t*)ptr + 48 );
        return (int)vecPtr->size();
    }

    __declspec(dllexport) int SQLConfig_get_query_size(int handleIndex) {
        auto* ptr = Hub::sqlArena.access({ (uint32_t)handleIndex });
        if (!ptr) return 0;
        auto* vecPtr = reinterpret_cast<std::string*>( (uint8_t*)ptr + 0 );
        return (int)vecPtr->size();
    }

    __declspec(dllexport) int SQLConfig_get_connectionString_size(int handleIndex) {
        auto* ptr = Hub::sqlArena.access({ (uint32_t)handleIndex });
        if (!ptr) return 0;
        auto* vecPtr = reinterpret_cast<std::string*>( (uint8_t*)ptr + 32 );
        return (int)vecPtr->size();
    }

    __declspec(dllexport) int ServerRequest_get_url_size(int handleIndex) {
        auto* ptr = Hub::requestArena.access({ (uint32_t)handleIndex });
        if (!ptr) return 0;
        auto* vecPtr = reinterpret_cast<std::string*>( (uint8_t*)ptr + 0 );
        return (int)vecPtr->size();
    }

    __declspec(dllexport) int ServerRequest_get_method_size(int handleIndex) {
        auto* ptr = Hub::requestArena.access({ (uint32_t)handleIndex });
        if (!ptr) return 0;
        auto* vecPtr = reinterpret_cast<std::string*>( (uint8_t*)ptr + 32 );
        return (int)vecPtr->size();
    }

    __declspec(dllexport) int ServerRequest_get_payload_size(int handleIndex) {
        auto* ptr = Hub::requestArena.access({ (uint32_t)handleIndex });
        if (!ptr) return 0;
        auto* vecPtr = reinterpret_cast<std::string*>( (uint8_t*)ptr + 64 );
        return (int)vecPtr->size();
    }

    __declspec(dllexport) double ServerResponse_get_statusCode(int handleIndex) {
        auto* ptr = Hub::responseArena.access({ (uint32_t)handleIndex });
        if (!ptr) return -1;
        auto* valPtr = reinterpret_cast<int32_t*>( (uint8_t*)ptr + 0 );
        return (double)*valPtr;
    }

    __declspec(dllexport) int ServerResponse_get_body_size(int handleIndex) {
        auto* ptr = Hub::responseArena.access({ (uint32_t)handleIndex });
        if (!ptr) return 0;
        auto* vecPtr = reinterpret_cast<std::string*>( (uint8_t*)ptr + 8 );
        return (int)vecPtr->size();
    }

    __declspec(dllexport) double CameraFrame_get_width(int handleIndex) {
        auto* ptr = Hub::frameArena.access({ (uint32_t)handleIndex });
        if (!ptr) return -1;
        auto* valPtr = reinterpret_cast<int32_t*>( (uint8_t*)ptr + 0 );
        return (double)*valPtr;
    }

    __declspec(dllexport) double CameraFrame_get_height(int handleIndex) {
        auto* ptr = Hub::frameArena.access({ (uint32_t)handleIndex });
        if (!ptr) return -1;
        auto* valPtr = reinterpret_cast<int32_t*>( (uint8_t*)ptr + 4 );
        return (double)*valPtr;
    }

    __declspec(dllexport) int CameraFrame_get_pixelBuffer_size(int handleIndex) {
        auto* ptr = Hub::frameArena.access({ (uint32_t)handleIndex });
        if (!ptr) return 0;
        auto* vecPtr = reinterpret_cast<std::vector<uint8_t>*>( (uint8_t*)ptr + 8 );
        return (int)vecPtr->size();
    }

}
