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
  static std::unique_ptr<AeEventLoop> InitEventLoop();
  void AeMain();
  /* used for sync read/write. Timeout is in milliseconds */
  int AeWait(int fd, int mask, long timeout) const;
  AeStatus AeCreateFileEvent(int fd, AeFileEvent* fe);
  AeStatus AeDeleteFileEvent(int fd, int mask);
  void AeCreateTimeEvent(AeTimeEvent* te);
  void AeProcessEvents();
  ~AeEventLoop();

 private:
  static constexpr const int EventsSize = 1024;
  explicit AeEventLoop(AeKqueue* kq);
  std::vector<AeFileEvent*> fileEvents;
  mutable AeTimeEvent* timeEventHead;
  const AeKqueue* aeApiState;
  mutable int max_fd;
  void ProcessTimeEvents() const;
};
}  // namespace ae
}  // namespace redis_simple
