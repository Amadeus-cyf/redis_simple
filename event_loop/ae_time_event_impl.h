#pragma once

#include <sys/time.h>

#include <ctime>

namespace redis_simple {
namespace ae {
template <typename T>
class AeTimeEventImpl : public AeTimeEvent {
 public:
  using aeTimeProc = int (*)(long long id, T* client_data);
  using aeTimeFinalizeProc = int (*)(T* client_data);
  static AeTimeEvent* Create(aeTimeProc time_proc,
                             aeTimeFinalizeProc finalize_proc, T* client_data) {
    static long long gen_id = 0;
    return new AeTimeEventImpl(gen_id++, time_proc, finalize_proc, client_data);
  }
  int CallTimeProc() override { return time_proc_(Id(), client_data_); }
  int CallTimeProc() const override { return time_proc_(Id(), client_data_); }
  bool HasTimeFinalizeProc() override { return finalize_proc_ != nullptr; }
  bool HasTimeFinalizeProc() const override {
    return finalize_proc_ != nullptr;
  }
  int CallTimeFinalizeProc() override { return finalize_proc_(client_data_); }
  int CallTimeFinalizeProc() const override {
    return finalize_proc_(client_data_);
  }
  void SetTimeProc(aeTimeProc proc) { time_proc_ = proc; }
  aeTimeFinalizeProc GetTimeFinalizeProc() { return finalize_proc_; }
  void SetTimeFinalizeProc(aeTimeFinalizeProc finalize_proc) {
    finalize_proc_ = finalize_proc;
  }
  T* ClientData() { return client_data_; }
  void SetClientData(T* client_data) { client_data_ = client_data; }

 private:
  explicit AeTimeEventImpl(long long id, aeTimeProc time_proc,
                           aeTimeFinalizeProc finalize_proc, T* client_data)
      : AeTimeEvent(id),
        time_proc_(time_proc),
        finalize_proc_(finalize_proc),
        client_data_(client_data) {}
  aeTimeProc time_proc_;
  aeTimeFinalizeProc finalize_proc_;
  T* client_data_;
};
}  // namespace ae
}  // namespace redis_simple
