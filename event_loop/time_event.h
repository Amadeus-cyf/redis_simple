#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

namespace redis_simple::event_loop {
class TimeEvent {
 public:
  using TimeCallback = std::function<int(int64_t)>;
  using FinalizeCallback = std::function<int()>;

  static std::unique_ptr<TimeEvent> Create(TimeCallback time_callback,
                                           FinalizeCallback finalize_callback) {
    if (!time_callback) {
      return nullptr;
    }
    static int64_t gen_id = 0;
    return std::unique_ptr<TimeEvent>(new TimeEvent(
        gen_id++, std::move(time_callback), std::move(finalize_callback)));
  }

  template <typename T>
  static std::unique_ptr<TimeEvent> Create(int (*time_callback)(int64_t, T*),
                                           int (*finalize_callback)(T*),
                                           T* client_data) {
    return Create(Wrap(time_callback, client_data),
                  Wrap(finalize_callback, client_data));
  }

  int64_t Id() const { return id_; }
  void SetId(int64_t id) { id_ = id; }
  void SetWhen(int64_t when) { when_ = when; }
  int64_t When() const { return when_; }
  bool HasFinalizeCallback() const {
    return static_cast<bool>(finalize_callback_);
  }
  int CallTimeCallback() const { return time_callback_(id_); }
  int CallFinalizeCallback() const { return finalize_callback_(); }

 private:
  explicit TimeEvent(int64_t id, TimeCallback time_callback,
                     FinalizeCallback finalize_callback)
      : id_(id),
        when_(0),
        time_callback_(std::move(time_callback)),
        finalize_callback_(std::move(finalize_callback)) {}

  template <typename T>
  static TimeCallback Wrap(int (*callback)(int64_t, T*), T* client_data) {
    if (callback == nullptr) {
      return nullptr;
    }
    return [callback, client_data](int64_t id) {
      return callback(id, client_data);
    };
  }

  template <typename T>
  static FinalizeCallback Wrap(int (*callback)(T*), T* client_data) {
    if (callback == nullptr) {
      return nullptr;
    }
    return [callback, client_data]() { return callback(client_data); };
  }

  int64_t id_;
  int64_t when_;
  TimeCallback time_callback_;
  FinalizeCallback finalize_callback_;
};
}  // namespace redis_simple::event_loop
