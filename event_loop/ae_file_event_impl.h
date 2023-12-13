#pragma once

#include "event_loop/ae_file_event.h"

namespace redis_simple {
namespace ae {
class AeEventLoop;

template <typename T>
class AeFileEventImpl : public AeFileEvent {
 public:
  using aeFileProc = AeEventStatus (*)(AeEventLoop* el, int fd, T* client_data,
                                       int mask);
  static AeFileEvent* create(aeFileProc rfile_proc, aeFileProc wfile_proc,
                             T* client_data, int mask) {
    return new AeFileEventImpl(rfile_proc, wfile_proc, client_data, mask);
  }
  void callReadProc(AeEventLoop* el, int fd, int mask) override {
    rfile_proc(el, fd, client_data, mask);
  }
  void callReadProc(AeEventLoop* el, int fd, int mask) const override {
    rfile_proc(el, fd, client_data, mask);
  }
  void callWriteProc(AeEventLoop* el, int fd, int mask) override {
    wfile_proc(el, fd, client_data, mask);
  }
  void callWriteProc(AeEventLoop* el, int fd, int mask) const override {
    wfile_proc(el, fd, client_data, mask);
  }
  void merge(const AeFileEvent* fe) override;
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
  explicit AeFileEventImpl(aeFileProc rfile_proc, aeFileProc wfile_proc,
                           T* client_data, int mask)
      : rfile_proc(rfile_proc),
        wfile_proc(wfile_proc),
        client_data(client_data),
        AeFileEvent(mask) {}
  aeFileProc rfile_proc;
  aeFileProc wfile_proc;
  T* client_data;
};

template <typename T>
void AeFileEventImpl<T>::merge(const AeFileEvent* fe) {
  if (!fe) {
    return;
  }
  const AeFileEventImpl<T>* feImpl = static_cast<const AeFileEventImpl<T>*>(fe);
  int mask = getMask();
  setMask(mask | fe->getMask());
  if (!hasRFileProc() && fe->hasRFileProc()) {
    rfile_proc = feImpl->getRFileProc();
  }
  if (!hasWFileProc() && fe->hasWFileProc()) {
    wfile_proc = feImpl->getWFileProc();
  }
}
}  // namespace ae
}  // namespace redis_simple
