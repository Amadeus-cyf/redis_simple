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
  static AeFileEvent* Create(aeFileProc rfile_proc, aeFileProc wfile_proc,
                             T* client_data, int mask) {
    return new AeFileEventImpl(rfile_proc, wfile_proc, client_data, mask);
  }
  void CallReadProc(AeEventLoop* el, int fd, int mask) override {
    rfile_proc(el, fd, client_data, mask);
  }
  void CallReadProc(AeEventLoop* el, int fd, int mask) const override {
    rfile_proc(el, fd, client_data, mask);
  }
  void CallWriteProc(AeEventLoop* el, int fd, int mask) override {
    wfile_proc(el, fd, client_data, mask);
  }
  void CallWriteProc(AeEventLoop* el, int fd, int mask) const override {
    wfile_proc(el, fd, client_data, mask);
  }
  void Merge(const AeFileEvent* fe) override;
  aeFileProc RFileProc() { return rfile_proc; }
  aeFileProc RFileProc() const { return rfile_proc; }
  void SetRFileProc(aeFileProc p) { rfile_proc = p; }
  aeFileProc WFileProc() { return wfile_proc; }
  aeFileProc WFileProc() const { return wfile_proc; }
  void SetWFileProc(aeFileProc p) { wfile_proc = p; }
  bool IsRWProcDiff() override { return rfile_proc != wfile_proc; }
  bool IsRWProcDiff() const override { return rfile_proc != wfile_proc; }
  T* ClientData() { return client_data; }
  T* ClientData() const { return client_data; }
  void ClientData(const T* data) { client_data = data; }
  bool HasRFileProc() override { return rfile_proc != nullptr; }
  bool HasRFileProc() const override { return rfile_proc != nullptr; }
  bool HasWFileProc() override { return wfile_proc != nullptr; }
  bool HasWFileProc() const override { return wfile_proc != nullptr; }

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
void AeFileEventImpl<T>::Merge(const AeFileEvent* fe) {
  if (!fe) {
    return;
  }
  const AeFileEventImpl<T>* feImpl = static_cast<const AeFileEventImpl<T>*>(fe);
  int mask = GetMask();
  SetMask(mask | fe->GetMask());
  if (!HasRFileProc() && fe->HasRFileProc()) {
    rfile_proc = feImpl->RFileProc();
  }
  if (!HasWFileProc() && fe->HasWFileProc()) {
    wfile_proc = feImpl->WFileProc();
  }
}
}  // namespace ae
}  // namespace redis_simple
