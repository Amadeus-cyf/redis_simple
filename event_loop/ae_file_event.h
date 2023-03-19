#pragma once

#include "event_loop/ae_file_event_base.h"

namespace redis_simple {
namespace ae {
enum class AeEventStatus {
  aeEventOK = 0,
  aeEventErr = -1,
};

class AeEventLoop;

template <typename T>
class AeFileEvent : public BaseAeFileEvent {
 public:
  using aeFileProc = AeEventStatus (*)(AeEventLoop* el, int fd, T* client_data,
                                       int mask);
  static AeFileEvent* create(aeFileProc rfile_proc, aeFileProc wfile_proc,
                             T* client_data, int mask) {
    return new AeFileEvent(rfile_proc, wfile_proc, client_data, mask);
  }
  void callReadProc(ae::AeEventLoop* el, int fd) override {
    rfile_proc(el, fd, client_data, getMask());
  }
  void callReadProc(ae::AeEventLoop* el, int fd) const override {
    rfile_proc(el, fd, client_data, getMask());
  }
  void callWriteProc(ae::AeEventLoop* el, int fd) override {
    wfile_proc(el, fd, client_data, getMask());
  }
  void callWriteProc(ae::AeEventLoop* el, int fd) const override {
    wfile_proc(el, fd, client_data, getMask());
  }
  aeFileProc getRFileProc() { return rfile_proc; }
  aeFileProc getRFileProc() const { return rfile_proc; }
  void setRFileProc(aeFileProc p) { rfile_proc = p; }
  aeFileProc getWFileProc() { return wfile_proc; }
  aeFileProc getWFileProc() const { return wfile_proc; }
  void setWFileProc(aeFileProc p) { wfile_proc = p; }
  bool isRWProcDiff() override { return rfile_proc != wfile_proc; }
  bool isRWProcDiff() const override { return rfile_proc != wfile_proc; }
  T* getClientData() { return client_data; }
  T* getClientData() const { return client_data; }
  void setClientData(const T* data) { client_data = data; }
  bool hasRFileProc() override { return rfile_proc != nullptr; }
  bool hasRFileProc() const override { return rfile_proc != nullptr; }
  bool hasWFileProc() override { return wfile_proc != nullptr; }
  bool hasWFileProc() const override { return wfile_proc != nullptr; }

 private:
  explicit AeFileEvent(aeFileProc rfile_proc, aeFileProc wfile_proc,
                       T* client_data, int mask)
      : rfile_proc(rfile_proc),
        wfile_proc(wfile_proc),
        client_data(client_data),
        BaseAeFileEvent(mask) {}
  aeFileProc rfile_proc;
  aeFileProc wfile_proc;
  T* client_data;
};
}  // namespace ae
}  // namespace redis_simple
