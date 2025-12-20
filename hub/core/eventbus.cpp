// =============================================================================
// EVENTBUS.CPP - Event Bus Implementation
// =============================================================================

#include "eventbus.h"
#include <algorithm>
#include <cstring>


static thread_local std::string tl_name;
static thread_local std::string tl_data;

extern "C" {

__declspec(dllexport) int hub_event_subscribe(int event_type,
                                              hub_event_callback callback) {
  if (!callback)
    return -1;
  return Hub::EventBus::instance().subscribe(
      static_cast<Hub::EventType>(event_type), callback);
}

__declspec(dllexport) int hub_event_subscribe_all(hub_event_callback callback) {
  if (!callback)
    return -1;
  return Hub::EventBus::instance().subscribeAll(callback);
}

__declspec(dllexport) void hub_event_unsubscribe(int subscription_id) {
  Hub::EventBus::instance().unsubscribe(subscription_id);
}

__declspec(dllexport) void hub_event_publish(int event_type, const char *name,
                                             const char *data) {
  Hub::EventBus::instance().publish(static_cast<Hub::EventType>(event_type),
                                    name ? name : "", data ? data : "");
}

__declspec(dllexport) int hub_event_poll(int *out_type, char *out_name,
                                         int name_size, char *out_data,
                                         int data_size) {
  Hub::Event event;
  if (!Hub::EventBus::instance().poll(event))
    return 0;

  if (out_type)
    *out_type = static_cast<int>(event.type);

  if (out_name && name_size > 0) {
    size_t len =
        (std::min)(static_cast<size_t>(name_size - 1), event.name.size());
    std::memcpy(out_name, event.name.c_str(), len);
    out_name[len] = '\0';
  }

  if (out_data && data_size > 0) {
    size_t len =
        (std::min)(static_cast<size_t>(data_size - 1), event.data.size());
    std::memcpy(out_data, event.data.c_str(), len);
    out_data[len] = '\0';
  }

  return 1;
}

__declspec(dllexport) int hub_event_pending_count() {
  return static_cast<int>(Hub::EventBus::instance().pendingCount());
}

__declspec(dllexport) void hub_event_clear() {
  Hub::EventBus::instance().clear();
}

} // extern "C"
