#pragma once

namespace redis_simple {
namespace ae {
enum class AeEventStatus {
  aeEventOK = 0,
  aeEventErr = -1,
};

class AeEventLoop;

using aeFileProc = AeEventStatus (*)(AeEventLoop* el, int fd, void* client_data,
                                     int mask);

class AeFileEvent {
 public:
  static AeFileEvent* create(aeFileProc rfile_proc, aeFileProc wfile_proc,
                             void* client_data, int mask) {
    return new AeFileEvent(rfile_proc, wfile_proc, client_data, mask);
  }
  aeFileProc getRFileProc() { return rfile_proc; }
  aeFileProc getRFileProc() const { return rfile_proc; }
  void setRFileProc(aeFileProc p) { rfile_proc = p; }
  aeFileProc getWFileProc() { return wfile_proc; }
  aeFileProc getWFileProc() const { return wfile_proc; }
  void setWFileProc(aeFileProc p) { wfile_proc = p; }
  void* getClientData() { return client_data; }
  void* getClientData() const { return client_data; }
  void setClientData(void* data) { client_data = data; }
  int getMask() { return mask; }
  int getMask() const { return mask; }
  void setMask(int m) { mask |= m; }
  bool hasRFileProc() { return rfile_proc != nullptr; }
  bool hasRFileProc() const { return rfile_proc != nullptr; }
  bool hasWFileProc() { return wfile_proc != nullptr; }
  bool hasWFileProc() const { return wfile_proc != nullptr; }

 private:
  explicit AeFileEvent(aeFileProc rfile_proc, aeFileProc wfile_proc,
                       void* client_data, int mask)
      : rfile_proc(rfile_proc),
        wfile_proc(wfile_proc),
        client_data(client_data),
        mask(mask) {}
  aeFileProc rfile_proc;
  aeFileProc wfile_proc;
  void* client_data;
  int mask;
};
}  // namespace ae
}  // namespace redis_simple
