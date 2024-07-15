#pragma once

#include "event_loop/ae_file_event.h"

namespace redis_simple {
namespace ae {
class AeEventLoop;

template <typename T>
class AeFileEventImpl : public AeFileEvent {
 public:
  using aeFileProc = AeEventStatus (*)(AeEventLoop* el, int fd, T* client_data_,
                                       int mask);
  static AeFileEvent* Create(aeFileProc rfile_proc_, aeFileProc wfile_proc_,
                             T* client_data_, int mask) {
    return new AeFileEventImpl(rfile_proc_, wfile_proc_, client_data_, mask);
  }
  void CallReadProc(AeEventLoop* el, int fd, int mask) const override {
    rfile_proc_(el, fd, client_data_, mask);
  }
  void CallWriteProc(AeEventLoop* el, int fd, int mask) const override {
    wfile_proc_(el, fd, client_data_, mask);
  }
  bool IsRWProcDiff() const override { return rfile_proc_ != wfile_proc_; }
  void Merge(const AeFileEvent* fe) override;
  aeFileProc RFileProc() const { return rfile_proc_; }
  void SetRFileProc(aeFileProc p) { rfile_proc_ = p; }
  aeFileProc WFileProc() const { return wfile_proc_; }
  void SetWFileProc(aeFileProc p) { wfile_proc_ = p; }
  T* ClientData() const { return client_data_; }
  void ClientData(const T* data) { client_data_ = data; }
  bool HasRFileProc() const override { return rfile_proc_ != nullptr; }
  bool HasWFileProc() const override { return wfile_proc_ != nullptr; }

 private:
  explicit AeFileEventImpl(aeFileProc rfile_proc_, aeFileProc wfile_proc_,
                           T* client_data_, int mask)
      : rfile_proc_(rfile_proc_),
        wfile_proc_(wfile_proc_),
        client_data_(client_data_),
        AeFileEvent(mask) {}
  aeFileProc rfile_proc_;
  aeFileProc wfile_proc_;
  T* client_data_;
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
    rfile_proc_ = feImpl->RFileProc();
  }
  if (!HasWFileProc() && fe->HasWFileProc()) {
    wfile_proc_ = feImpl->WFileProc();
  }
}
}  // namespace ae
}  // namespace redis_simple
