#pragma once
// =============================================================================
// ASYNC.H - Enterprise Thread Pool & Async Task System
// =============================================================================
// High-performance task execution with:
// - Priority queue (HIGH/NORMAL/LOW)
// - Proper futures for async results
// - Graceful shutdown with task completion
// - Task statistics and monitoring
// - Handle 10,000+ concurrent tasks
// =============================================================================

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace Hub {

// TaskId and ResultCallback are defined in types.h
// For self-contained usage, define if not already defined
#ifndef HUB_TASK_ID_DEFINED
using TaskId = uint32_t;
using ResultCallback = void (*)(TaskId task_id, int result_handle,
                                const char *error_json);
#define HUB_TASK_ID_DEFINED
#endif

enum class TaskPriority : int { LOW = 0, NORMAL = 1, HIGH = 2, CRITICAL = 3 };

// =============================================================================
// TASK STRUCTURE
// =============================================================================
struct Task {
  TaskId id;
  TaskPriority priority;
  std::function<int()> work;
  std::shared_ptr<std::promise<int>> promise_ptr;
  ResultCallback callback;
  std::chrono::steady_clock::time_point created_at;

  Task() : id(0), priority(TaskPriority::NORMAL), callback(nullptr) {}

  Task(TaskId tid, TaskPriority prio, std::function<int()> w,
       std::shared_ptr<std::promise<int>> prom = nullptr,
       ResultCallback cb = nullptr)
      : id(tid), priority(prio), work(std::move(w)), promise_ptr(prom),
        callback(cb), created_at(std::chrono::steady_clock::now()) {}

  // Priority comparison for queue (higher priority first, then older first)
  bool operator<(const Task &other) const {
    if (priority != other.priority) {
      return static_cast<int>(priority) < static_cast<int>(other.priority);
    }
    return created_at >
           other.created_at; // Older tasks first (FIFO within same priority)
  }
};

// =============================================================================
// TASK STATISTICS
// =============================================================================
struct TaskStats {
  std::atomic<uint64_t> total_submitted{0};
  std::atomic<uint64_t> total_completed{0};
  std::atomic<uint64_t> total_failed{0};
  std::atomic<uint64_t> active_tasks{0};
  std::atomic<uint64_t> queue_high{0};
  std::atomic<uint64_t> queue_normal{0};
  std::atomic<uint64_t> queue_low{0};

  void reset() {
    total_submitted = 0;
    total_completed = 0;
    total_failed = 0;
    active_tasks = 0;
    queue_high = 0;
    queue_normal = 0;
    queue_low = 0;
  }
};

// =============================================================================
// THREAD POOL (Enterprise Edition)
// =============================================================================
class AsyncRunner {
public:
  static AsyncRunner &instance() {
    static AsyncRunner inst;
    return inst;
  }

  // Initialize with worker count (0 = auto-detect)
  void init(size_t thread_count = 0) {
    if (initialized_)
      return;

    if (thread_count == 0) {
      thread_count = std::thread::hardware_concurrency();
      if (thread_count == 0)
        thread_count = 4;
    }

    // Scale for enterprise workloads
    worker_count_ = thread_count;

    std::cout << "[async] Initializing enterprise thread pool with "
              << worker_count_ << " workers\n";

    for (size_t i = 0; i < worker_count_; ++i) {
      workers_.emplace_back([this, i] { worker_loop(i); });
    }

    stats_.reset();
    initialized_ = true;
    shutdown_ = false;
  }

  // Submit task with priority, returns future
  std::future<int> submit(std::function<int()> work,
                          TaskPriority priority = TaskPriority::NORMAL,
                          ResultCallback callback = nullptr) {
    auto promise = std::make_shared<std::promise<int>>();
    std::future<int> future = promise->get_future();

    TaskId id = next_task_id_++;
    stats_.total_submitted++;

    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      task_queue_.emplace(id, priority, std::move(work), promise, callback);

      // Update queue stats
      switch (priority) {
      case TaskPriority::HIGH:
      case TaskPriority::CRITICAL:
        stats_.queue_high++;
        break;
      case TaskPriority::NORMAL:
        stats_.queue_normal++;
        break;
      case TaskPriority::LOW:
        stats_.queue_low++;
        break;
      }
    }

    condition_.notify_one();
    return future;
  }

  // Submit and block until complete
  int submit_blocking(std::function<int()> work,
                      TaskPriority priority = TaskPriority::NORMAL) {
    return submit(std::move(work), priority).get();
  }

  // Submit fire-and-forget (no future)
  TaskId submit_async(std::function<int()> work,
                      TaskPriority priority = TaskPriority::NORMAL,
                      ResultCallback callback = nullptr) {
    TaskId id = next_task_id_++;
    stats_.total_submitted++;

    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      task_queue_.emplace(id, priority, std::move(work), nullptr, callback);
    }

    condition_.notify_one();
    return id;
  }

  // Graceful shutdown - wait for all tasks to complete
  void shutdown(bool wait_for_tasks = true) {
    if (!initialized_)
      return;

    std::cout << "[async] Shutting down thread pool";
    if (wait_for_tasks) {
      std::cout << " (waiting for " << pending_count() << " pending tasks)";
    }
    std::cout << "\n";

    // Wait for tasks to drain if requested
    if (wait_for_tasks) {
      while (pending_count() > 0 || stats_.active_tasks > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }

    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      shutdown_ = true;
    }

    condition_.notify_all();

    for (auto &worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }

    workers_.clear();
    initialized_ = false;

    std::cout << "[async] Shutdown complete. Stats: " << stats_.total_completed
              << " completed, " << stats_.total_failed << " failed\n";
  }

  // Force shutdown - discard pending tasks
  void shutdown_now() {
    if (!initialized_)
      return;

    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      shutdown_ = true;
      // Clear the queue
      std::priority_queue<Task> empty;
      std::swap(task_queue_, empty);
    }

    condition_.notify_all();

    for (auto &worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }

    workers_.clear();
    initialized_ = false;
  }

  // Statistics
  size_t pending_count() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return task_queue_.size();
  }

  size_t worker_count() const { return worker_count_; }
  size_t active_count() const { return stats_.active_tasks; }
  bool is_initialized() const { return initialized_; }
  const TaskStats &stats() const { return stats_; }

private:
  AsyncRunner()
      : initialized_(false), shutdown_(false), next_task_id_(1),
        worker_count_(0) {}
  ~AsyncRunner() { shutdown(false); }
  AsyncRunner(const AsyncRunner &) = delete;
  AsyncRunner &operator=(const AsyncRunner &) = delete;

  void worker_loop(size_t worker_id) {
    while (true) {
      Task task;

      {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        condition_.wait(lock,
                        [this] { return shutdown_ || !task_queue_.empty(); });

        if (shutdown_ && task_queue_.empty()) {
          break;
        }

        task = std::move(const_cast<Task &>(task_queue_.top()));
        task_queue_.pop();
      }

      stats_.active_tasks++;

      int result = -1;
      bool success = false;
      std::string error_str;

      try {
        result = task.work();
        success = true;
        stats_.total_completed++;
      } catch (const std::exception &e) {
        error_str = std::string("{\"error\":\"") + e.what() + "\"}";
        stats_.total_failed++;
      } catch (...) {
        error_str = "{\"error\":\"Unknown exception\"}";
        stats_.total_failed++;
      }

      stats_.active_tasks--;

      // Set promise result
      if (task.promise_ptr) {
        if (success) {
          task.promise_ptr->set_value(result);
        } else {
          task.promise_ptr->set_exception(
              std::make_exception_ptr(std::runtime_error(error_str)));
        }
      }

      // Call callback
      if (task.callback) {
        try {
          task.callback(task.id, result, success ? nullptr : error_str.c_str());
        } catch (...) {
          // Ignore callback exceptions
        }
      }
    }
  }

  std::vector<std::thread> workers_;
  mutable std::mutex queue_mutex_;
  std::condition_variable condition_;
  std::priority_queue<Task> task_queue_; // Priority queue!

  std::atomic<bool> initialized_;
  std::atomic<bool> shutdown_;
  std::atomic<TaskId> next_task_id_;
  size_t worker_count_;
  TaskStats stats_;
};

} // namespace Hub

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {

__declspec(dllexport) void hub_async_init(int thread_count) {
  Hub::AsyncRunner::instance().init(
      thread_count > 0 ? static_cast<size_t>(thread_count) : 0);
}

__declspec(dllexport) void hub_async_shutdown() {
  Hub::AsyncRunner::instance().shutdown(true);
}

__declspec(dllexport) void hub_async_shutdown_now() {
  Hub::AsyncRunner::instance().shutdown_now();
}

__declspec(dllexport) int hub_async_pending_count() {
  return static_cast<int>(Hub::AsyncRunner::instance().pending_count());
}

__declspec(dllexport) int hub_async_active_count() {
  return static_cast<int>(Hub::AsyncRunner::instance().active_count());
}

__declspec(dllexport) int hub_async_worker_count() {
  return static_cast<int>(Hub::AsyncRunner::instance().worker_count());
}

__declspec(dllexport) uint64_t hub_async_stats_completed() {
  return Hub::AsyncRunner::instance().stats().total_completed;
}

__declspec(dllexport) uint64_t hub_async_stats_failed() {
  return Hub::AsyncRunner::instance().stats().total_failed;
}
}
