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
  aeOK = 0,
  aeErr = -1,
};

enum class AeEventStatus {
  aeEventOK = 0,
  aeEventErr = -1,
};

// Wait for milliseconds until the given file descriptor is
// readable/writable/exception. Return the mask indicating if the given file
// descriptor is readable/writeable. Used for sync read/write in Connection.
int AeWait(int fd, int mask, long timeout);

// Event loop
class AeEventLoop {
 public:
  static AeEventLoop* InitEventLoop();
  void AeMain();
  AeStatus AeCreateFileEvent(int fd, AeFileEvent* fe);
  AeStatus AeDeleteFileEvent(int fd, int mask);
  void AeCreateTimeEvent(AeTimeEvent* te);
  void AeProcessEvents();
  ~AeEventLoop();

 private:
  static constexpr const int eventSize = 1024;
  explicit AeEventLoop(AeKqueue* kq);
  void ProcessFileEvents();
  void ProcessTimeEvents() const;
  std::vector<AeFileEvent*> file_events_;
  mutable AeTimeEvent* time_event_head_;
  const AeKqueue* ae_api_state_;
  mutable int max_fd_;
};
}  // namespace ae
}  // namespace redis_simple
