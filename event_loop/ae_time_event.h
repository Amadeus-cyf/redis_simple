#pragma once

#include <ctime>
#include <functional>
#include <utility>

namespace redis_simple {
namespace ae {
class TimeEvent {
 public:
  using TimeCallback = std::function<int(long long)>;
  using FinalizeCallback = std::function<int()>;

  static TimeEvent* Create(TimeCallback time_callback,
                           FinalizeCallback finalize_callback) {
    static long long gen_id = 0;
    return new TimeEvent(gen_id++, std::move(time_callback),
                         std::move(finalize_callback));
  }

  template <typename T>
  static TimeEvent* Create(int (*time_callback)(long long, T*),
                           int (*finalize_callback)(T*), T* client_data) {
    return Create(Wrap(time_callback, client_data),
                  Wrap(finalize_callback, client_data));
  }

  long long Id() const { return id_; }
  void SetId(long long id) { id_ = id; }
  void SetNext(TimeEvent* next) { next_ = next; }
  void DeleteNext() { next_ = nullptr; }
  TimeEvent* Next() const { return next_; }
  void SetPrev(TimeEvent* prev) { prev_ = prev; }
  void DeletePrev() { prev_ = nullptr; }
  TimeEvent* Prev() const { return prev_; }
  void SetWhen(time_t when) { when_ = when; }
  time_t When() const { return when_; }
  bool HasFinalizeCallback() const {
    return static_cast<bool>(finalize_callback_);
  }
  int CallTimeCallback() const { return time_callback_(id_); }
  int CallFinalizeCallback() const { return finalize_callback_(); }

 private:
  explicit TimeEvent(long long id, TimeCallback time_callback,
                     FinalizeCallback finalize_callback)
      : id_(id),
        when_(0),
        next_(nullptr),
        prev_(nullptr),
        time_callback_(std::move(time_callback)),
        finalize_callback_(std::move(finalize_callback)) {}

  template <typename T>
  static TimeCallback Wrap(int (*callback)(long long, T*), T* client_data) {
    if (callback == nullptr) {
      return nullptr;
    }
    return [callback, client_data](long long id) {
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

  long long id_;
  time_t when_;
  TimeEvent *next_, *prev_;
  TimeCallback time_callback_;
  FinalizeCallback finalize_callback_;
};
}  // namespace ae
}  // namespace redis_simple
