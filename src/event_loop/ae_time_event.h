#pragma once

#include <sys/time.h>

#include <ctime>

namespace redis_simple {
namespace ae {
using aeTimeProc = int (*)(long long id, void* client_data);
using aeTimeFinalizeProc = int (*)(void* client_data);

class AeTimeEvent {
 public:
  static AeTimeEvent* createAeTimeEvent(aeTimeProc time_proc,
                                        aeTimeFinalizeProc finalize_proc,
                                        void* client_data) {
    static long long id = 0;
    return new AeTimeEvent(id++, time_proc, finalize_proc, client_data);
  }
  static void freeAeTimeEvent(const AeTimeEvent* te) { delete te; }
  aeTimeProc getTimeProc() { return time_proc; }
  long long getId() { return id; }
  void setId(long long tid) { id = tid; }
  void setTimeProc(aeTimeProc proc) { time_proc = proc; }
  aeTimeFinalizeProc getTimeFinalizeProc() { return finalize_proc; }
  void setTimeFinalizeProc(aeTimeFinalizeProc proc) { finalize_proc = proc; }
  bool hasTimeFinalizeProc() { return finalize_proc != nullptr; }
  void setNext(AeTimeEvent* next) { this->next = next; }
  void delNext() { next = nullptr; }
  AeTimeEvent* getNext() { return next; }
  void setPrev(AeTimeEvent* prev) { this->prev = prev; }
  void delPrev() { prev = nullptr; }
  AeTimeEvent* getPrev() { return prev; }
  void* getClientData() { return client_data; }
  void setClientData(void* data) { client_data = data; }
  int64_t getWhen() { return when; }
  void setWhen(time_t w) { when = w; }

 private:
  explicit AeTimeEvent(long long id, aeTimeProc time_proc,
                       aeTimeFinalizeProc finalize_proc, void* client_data)
      : id(id),
        time_proc(time_proc),
        finalize_proc(finalize_proc),
        client_data(client_data),
        when(0) {}
  long long id;
  aeTimeProc time_proc;
  aeTimeFinalizeProc finalize_proc;
  AeTimeEvent* next;
  AeTimeEvent* prev;
  void* client_data;
  int64_t when;
};
}  // namespace ae
}  // namespace redis_simple
