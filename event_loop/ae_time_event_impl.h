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
  int CallTimeProc() override { return time_proc(Id(), client_data); }
  int CallTimeProc() const override { return time_proc(Id(), client_data); }
  bool HasTimeFinalizeProc() override { return finalize_proc != nullptr; }
  bool HasTimeFinalizeProc() const override { return finalize_proc != nullptr; }
  int CallTimeFinalizeProc() override { return finalize_proc(client_data); }
  int CallTimeFinalizeProc() const override {
    return finalize_proc(client_data);
  }
  void SetTimeProc(aeTimeProc proc) { time_proc = proc; }
  aeTimeFinalizeProc GetTimeFinalizeProc() { return finalize_proc; }
  void SetTimeFinalizeProc(aeTimeFinalizeProc proc) { finalize_proc = proc; }
  T* ClientData() { return client_data; }
  void SetClientData(T* data) { client_data = data; }

 private:
  explicit AeTimeEventImpl(long long id, aeTimeProc time_proc,
                           aeTimeFinalizeProc finalize_proc, T* client_data)
      : AeTimeEvent(id),
        time_proc(time_proc),
        finalize_proc(finalize_proc),
        client_data(client_data) {}
  aeTimeProc time_proc;
  aeTimeFinalizeProc finalize_proc;
  T* client_data;
};
}  // namespace ae
}  // namespace redis_simple
