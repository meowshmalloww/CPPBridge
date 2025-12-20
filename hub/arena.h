#pragma once
#include <vector>
#include <mutex>
#include "schema.h"

namespace Hub {

    template <typename T>
    class Arena {
        struct Slot {
            T data;
            bool active = false;
        };
        std::vector<Slot> storage;
        std::vector<uint32_t> freeIndices;
        std::mutex mtx;

    public:
        Handle insert(const T& item) {
            std::lock_guard<std::mutex> lock(mtx);
            uint32_t index;
            if (!freeIndices.empty()) {
                index = freeIndices.back();
                freeIndices.pop_back();
            } else {
                index = (uint32_t)storage.size();
                storage.resize(index + 1);
            }
            storage[index].data = item;
            storage[index].active = true;
            return { index };
        }

        T* access(Handle h) {
            if (h.index >= storage.size()) return nullptr;
            return &storage[h.index].data;
        }

        void release(Handle h) {
            std::lock_guard<std::mutex> lock(mtx);
            if (h.index < storage.size() && storage[h.index].active) {
                storage[h.index].active = false;
                freeIndices.push_back(h.index);
            }
        }
    };

    // Global Storage for our Backend Features
    extern Arena<SQLConfig> sqlArena;
    extern Arena<BigData> bigDataArena;
    extern Arena<ServerRequest> requestArena;
    extern Arena<ServerResponse> responseArena;
    extern Arena<CameraFrame> frameArena; // Keep for compatibility
}