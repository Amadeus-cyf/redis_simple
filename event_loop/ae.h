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

enum class AeEventStatus {
  aeEventOK = 0,
  aeEventErr = -1,
};

class AeEventLoop {
 public:
  static std::unique_ptr<AeEventLoop> initEventLoop();
  void aeMain();
  /* used for sync read/write. Timeout is in milliseconds */
  int aeWait(int fd, int mask, long timeout) const;
  AeStatus aeCreateFileEvent(int fd, AeFileEvent* fe);
  AeStatus aeDeleteFileEvent(int fd, int mask);
  void aeCreateTimeEvent(AeTimeEvent* te);
  void aeProcessEvents();
  ~AeEventLoop();

 private:
  static constexpr const int EventsSize = 1024;
  explicit AeEventLoop(AeKqueue* kq);
  std::vector<AeFileEvent*> fileEvents;
  mutable AeTimeEvent* timeEventHead;
  const AeKqueue* aeApiState;
  mutable int max_fd;
  void processTimeEvents() const;
};
}  // namespace ae
}  // namespace redis_simple
