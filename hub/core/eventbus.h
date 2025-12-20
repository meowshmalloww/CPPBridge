#pragma once
// =============================================================================
// EVENTBUS.H - Event Bus for Push Notifications
// =============================================================================
// Enables C++ to push events to frontends (Python/JS/Flutter).
// Features:
// - Event subscription by type
// - Callback-based notification
// - Event queue for polling
// - Thread-safe design
// =============================================================================

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace Hub {

// =============================================================================
// EVENT TYPES
// =============================================================================
enum class EventType : int {
  NONE = 0,
  // System events
  INIT_COMPLETE = 1,
  SHUTDOWN = 2,

  // File events
  FILE_CREATED = 10,
  FILE_MODIFIED = 11,
  FILE_DELETED = 12,

  // Network events
  HTTP_RESPONSE = 20,
  WS_MESSAGE = 21,
  WS_CONNECTED = 22,
  WS_DISCONNECTED = 23,

  // Task events
  TASK_STARTED = 30,
  TASK_PROGRESS = 31,
  TASK_COMPLETED = 32,
  TASK_FAILED = 33,

  // Custom events (100+)
  CUSTOM = 100
};

// =============================================================================
// EVENT STRUCTURE
// =============================================================================
struct Event {
  EventType type = EventType::NONE;
  int id = 0;
  std::string name;
  std::string data; // JSON payload
  int64_t timestamp = 0;

  Event()
      : timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count()) {}

  Event(EventType t, const std::string &n, const std::string &d = "")
      : type(t), name(n), data(d),
        timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count()) {}
};

// =============================================================================
// EVENT CALLBACK TYPE
// =============================================================================
using EventCallback = void (*)(int event_type, const char *name,
                               const char *data);

// =============================================================================
// EVENT BUS
// =============================================================================
class EventBus {
public:
  static EventBus &instance() {
    static EventBus inst;
    return inst;
  }

  // Subscribe to events
  int subscribe(EventType type, EventCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    int subId = nextSubId_++;
    subscriptions_[subId] = {type, callback};
    return subId;
  }

  // Subscribe to all events
  int subscribeAll(EventCallback callback) {
    return subscribe(EventType::NONE, callback);
  }

  // Unsubscribe
  void unsubscribe(int subscriptionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.erase(subscriptionId);
  }

  // Publish event
  void publish(const Event &event) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Add to queue for polling
    eventQueue_.push(event);
    if (eventQueue_.size() > maxQueueSize_) {
      eventQueue_.pop();
    }

    // Notify subscribers
    for (const auto &[id, sub] : subscriptions_) {
      if (sub.type == EventType::NONE || sub.type == event.type) {
        if (sub.callback) {
          try {
            sub.callback(static_cast<int>(event.type), event.name.c_str(),
                         event.data.c_str());
          } catch (...) {
            // Ignore callback exceptions
          }
        }
      }
    }

    cv_.notify_all();
  }

  // Publish helper
  void publish(EventType type, const std::string &name,
               const std::string &data = "") {
    publish(Event(type, name, data));
  }

  // Poll for next event (non-blocking)
  bool poll(Event &out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (eventQueue_.empty())
      return false;
    out = eventQueue_.front();
    eventQueue_.pop();
    return true;
  }

  // Wait for next event (blocking with timeout)
  bool wait(Event &out, int timeoutMs = 1000) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                     [this] { return !eventQueue_.empty(); })) {
      out = eventQueue_.front();
      eventQueue_.pop();
      return true;
    }
    return false;
  }

  // Check if events pending
  size_t pendingCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return eventQueue_.size();
  }

  // Clear all events
  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::queue<Event> empty;
    std::swap(eventQueue_, empty);
  }

private:
  EventBus() = default;

  struct Subscription {
    EventType type;
    EventCallback callback;
  };

  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::map<int, Subscription> subscriptions_;
  std::queue<Event> eventQueue_;
  std::atomic<int> nextSubId_{1};
  size_t maxQueueSize_ = 1000;
};

// =============================================================================
// CONVENIENCE FUNCTIONS
// =============================================================================
inline void emit(EventType type, const std::string &name,
                 const std::string &data = "") {
  EventBus::instance().publish(type, name, data);
}

inline void emit_task_progress(int taskId, int progress,
                               const std::string &message = "") {
  std::string data = "{\"taskId\":" + std::to_string(taskId) +
                     ",\"progress\":" + std::to_string(progress) +
                     ",\"message\":\"" + message + "\"}";
  EventBus::instance().publish(EventType::TASK_PROGRESS, "task_progress", data);
}

inline void emit_task_complete(int taskId, const std::string &result = "") {
  std::string data = "{\"taskId\":" + std::to_string(taskId) +
                     ",\"result\":\"" + result + "\"}";
  EventBus::instance().publish(EventType::TASK_COMPLETED, "task_complete",
                               data);
}

} // namespace Hub

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
// Event callback type for FFI
typedef void (*hub_event_callback)(int event_type, const char *name,
                                   const char *data);

__declspec(dllexport) int hub_event_subscribe(int event_type,
                                              hub_event_callback callback);
__declspec(dllexport) int hub_event_subscribe_all(hub_event_callback callback);
__declspec(dllexport) void hub_event_unsubscribe(int subscription_id);
__declspec(dllexport) void hub_event_publish(int event_type, const char *name,
                                             const char *data);
__declspec(dllexport) int hub_event_poll(int *out_type, char *out_name,
                                         int name_size, char *out_data,
                                         int data_size);
__declspec(dllexport) int hub_event_pending_count();
__declspec(dllexport) void hub_event_clear();
}
