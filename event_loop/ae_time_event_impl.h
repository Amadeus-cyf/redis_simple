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
  static AeTimeEvent* create(aeTimeProc time_proc,
                             aeTimeFinalizeProc finalize_proc, T* client_data) {
    static long long gen_id = 0;
    return new AeTimeEventImpl(gen_id++, time_proc, finalize_proc, client_data);
  }
  int callTimeProc() override { return time_proc(getId(), client_data); }
  bool hasTimeFinalizeProc() override { return finalize_proc != nullptr; }
  int callTimeFinalizeProc() override { return finalize_proc(client_data); }
  void setTimeProc(aeTimeProc proc) { time_proc = proc; }
  aeTimeFinalizeProc getTimeFinalizeProc() { return finalize_proc; }
  void setTimeFinalizeProc(aeTimeFinalizeProc proc) { finalize_proc = proc; }
  T* getClientData() { return client_data; }
  void setClientData(T* data) { client_data = data; }

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
