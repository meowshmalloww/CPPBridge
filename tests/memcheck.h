#pragma once
// =============================================================================
// MEMCHECK.H - Memory Leak Detection
// =============================================================================
// Tracks allocations and detects leaks using Windows CRT debug functions.
// =============================================================================

#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>
#endif

#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Hub {
namespace MemCheck {

// =============================================================================
// ALLOCATION TRACKER
// =============================================================================
class AllocationTracker {
public:
  static AllocationTracker &instance() {
    static AllocationTracker inst;
    return inst;
  }

  void init() {
#ifdef _WIN32
#ifdef _DEBUG
    // Enable CRT memory leak detection
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
#endif
#endif
    initialized_ = true;
  }

  void trackAlloc(void *ptr, size_t size, const char *file, int line) {
    std::lock_guard<std::mutex> lock(mutex_);
    allocations_[ptr] = {size, file ? file : "unknown", line};
    totalAllocated_ += size;
    currentAllocated_ += size;
    allocationCount_++;
  }

  void trackFree(void *ptr) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = allocations_.find(ptr);
    if (it != allocations_.end()) {
      currentAllocated_ -= it->second.size;
      allocations_.erase(it);
      freeCount_++;
    }
  }

  void report() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::cout << "\n=== Memory Report ===\n";
    std::cout << "Total allocated: " << totalAllocated_ << " bytes\n";
    std::cout << "Currently allocated: " << currentAllocated_ << " bytes\n";
    std::cout << "Allocation count: " << allocationCount_ << "\n";
    std::cout << "Free count: " << freeCount_ << "\n";

    if (!allocations_.empty()) {
      std::cout << "\nPotential leaks (" << allocations_.size() << "):\n";
      int shown = 0;
      for (auto it = allocations_.begin(); it != allocations_.end(); ++it) {
        void *ptr = it->first;
        const AllocInfo &info = it->second;
        std::cout << "  " << ptr << ": " << info.size << " bytes";
        std::cout << " (" << info.file << ":" << info.line << ")\n";
        if (++shown >= 10) {
          std::cout << "  ... and " << (allocations_.size() - 10) << " more\n";
          break;
        }
      }
    } else {
      std::cout << "\nNo memory leaks detected!\n";
    }
  }

  size_t leakCount() const { return allocations_.size(); }
  size_t currentBytes() const { return currentAllocated_; }
  bool isInitialized() const { return initialized_; }

private:
  struct AllocInfo {
    size_t size;
    std::string file;
    int line;
  };

  std::mutex mutex_;
  std::unordered_map<void *, AllocInfo> allocations_;
  std::atomic<size_t> totalAllocated_{0};
  std::atomic<size_t> currentAllocated_{0};
  std::atomic<size_t> allocationCount_{0};
  std::atomic<size_t> freeCount_{0};
  bool initialized_ = false;
};

// Convenience functions
inline void init_memcheck() { AllocationTracker::instance().init(); }

inline void report_memcheck() { AllocationTracker::instance().report(); }

inline size_t leak_count() { return AllocationTracker::instance().leakCount(); }

} // namespace MemCheck
} // namespace Hub

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
inline void hub_memcheck_init() { Hub::MemCheck::init_memcheck(); }
inline void hub_memcheck_report() { Hub::MemCheck::report_memcheck(); }
inline int hub_memcheck_leak_count() {
  return static_cast<int>(Hub::MemCheck::leak_count());
}
}
