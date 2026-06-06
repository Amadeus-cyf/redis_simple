#pragma once

#include <sys/time.h>

#include <ctime>

#include "event_loop/ae_time_event.h"

namespace redis_simple {
namespace ae {
template <typename T>
class TimeEventImpl : public TimeEvent {
 public:
  using TimeCallback = int (*)(long long id, T* client_data);
  using FinalizeCallback = int (*)(T* client_data);
  static TimeEvent* Create(TimeCallback time_proc,
                           FinalizeCallback finalize_proc, T* client_data) {
    static long long gen_id = 0;
    return new TimeEventImpl(gen_id++, time_proc, finalize_proc, client_data);
  }
  int CallTimeCallback() const override {
    return time_proc_(Id(), client_data_);
  }
  bool HasFinalizeCallback() const override {
    return finalize_proc_ != nullptr;
  }
  int CallFinalizeCallback() const override {
    return finalize_proc_(client_data_);
  }
  void SetTimeCallback(TimeCallback proc) { time_proc_ = proc; }
  FinalizeCallback GetFinalizeCallback() const { return finalize_proc_; }
  void SetFinalizeCallback(FinalizeCallback finalize_proc) {
    finalize_proc_ = finalize_proc;
  }
  T* ClientData() const { return client_data_; }
  void SetClientData(T* client_data) { client_data_ = client_data; }

 private:
  explicit TimeEventImpl(long long id, TimeCallback time_proc,
                         FinalizeCallback finalize_proc, T* client_data)
      : TimeEvent(id),
        time_proc_(time_proc),
        finalize_proc_(finalize_proc),
        client_data_(client_data) {}
  TimeCallback time_proc_;
  FinalizeCallback finalize_proc_;
  T* client_data_;
};
}  // namespace ae
}  // namespace redis_simple
