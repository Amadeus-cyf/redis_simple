#pragma once

#include <stdlib.h>

#include "ae_file_event.h"
#include "ae_file_event_base.h"
#include "ae_kqueue.h"
#include "ae_time_event.h"

namespace redis_simple {
namespace ae {
enum AeFlags {
  aeReadable = 1,
  aeWritable = 1 << 1,
  aeBarrier = 1 << 2,
  aeNoMore = -1,
  aeDeleteEventId = -1024,
};

enum AeStatus {
  ae_ok = 0,
  ae_err = -1,
};

class AeEventLoop {
 public:
  static std::unique_ptr<AeEventLoop> initEventLoop();
  void aeMain();
  /* used for sync read/write. Timeout is in milliseconds */
  int aeWait(int fd, int mask, long timeout) const;
  template <typename T>
  AeStatus aeCreateFileEvent(int fd, AeFileEvent<T>* fe);
  AeStatus aeDeleteFileEvent(int fd, int mask);
  void aeCreateTimeEvent(AeTimeEvent* te);
  void aeProcessEvents();
  ~AeEventLoop();

 private:
  static constexpr const int EventsSize = 1024;
  explicit AeEventLoop(AeKqueue* kq);
  std::vector<BaseAeFileEvent*> fileEvents;
  mutable AeTimeEvent* timeEventHead;
  const AeKqueue* aeApiState;
  mutable int max_fd;
  void processTimeEvents() const;
};

template <typename T>
AeStatus AeEventLoop::aeCreateFileEvent(int fd, AeFileEvent<T>* fe) {
  printf("create events for fd = %d, mask = %d\n", fd, fe->getMask());
  if (fe == nullptr) {
    return ae_err;
  }
  if (fd < 0 || fd >= EventsSize) {
    throw("file descriptor out of range");
  }
  int mask = 0;
  if (fileEvents[fd] == nullptr) {
    max_fd = std::max(max_fd, fd);
    printf("add new event\n");
    fileEvents[fd] = fe;
    mask = fe->getMask();
  } else {
    /* if a file event already exists, merge the event */
    AeFileEvent<T>* e = static_cast<AeFileEvent<T>*>(fileEvents[fd]);
    mask = e->getMask() | fe->getMask();
    fe->setMask(mask);
    if (e->hasRFileProc() && !fe->hasRFileProc()) {
      fe->setRFileProc(e->getRFileProc());
    }
    if (e->hasWFileProc() && !fe->hasWFileProc()) {
      fe->setWFileProc(e->getWFileProc());
    }
    delete e;
    fileEvents[fd] = fe;
  }
  return aeApiState->aeApiAddEvent(fd, mask) < 0 ? ae_err : ae_ok;
}
}  // namespace ae
}  // namespace redis_simple
