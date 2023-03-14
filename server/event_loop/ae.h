#pragma once

#include <stdlib.h>

#include "ae_file_event.h"
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
  void aeMain() const;
  /* used for sync read/write. Timeout is in milliseconds */
  int aeWait(int fd, int mask, long timeout) const;
  AeStatus aeCreateFileEvent(int fd, AeFileEvent* fe) const;
  AeStatus aeDelFileEvent(int fd, int mask) const;
  void aeCreateTimeEvent(AeTimeEvent* te) const;
  void aeProcessEvents() const;
  ~AeEventLoop();

 private:
  static constexpr const int EventsSize = 1024;
  explicit AeEventLoop(AeKqueue* kq);
  AeFileEvent** fileEvents;
  mutable AeTimeEvent* timeEventHead;
  AeKqueue* aeApiState;
  mutable int max_fd;
  void processTimeEvents() const;
};
}  // namespace ae
}  // namespace redis_simple
