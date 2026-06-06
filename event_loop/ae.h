#pragma once

#include <stdlib.h>

#include "ae_file_event.h"
#include "ae_kqueue.h"
#include "ae_time_event.h"

namespace redis_simple {
namespace ae {
enum EventFlag {
  kReadable = 1,
  kWritable = 1 << 1,
  kBarrier = 1 << 2,
  kNoMore = -1,
  kDeleteEventId = -1024,
};

enum EventLoopStatus {
  kOk = 0,
  kError = -1,
};

enum class EventHandlerStatus {
  kOk = 0,
  kError = -1,
};

// Wait for milliseconds until the given file descriptor is
// readable/writable/exception. Return the mask indicating if the given file
// descriptor is ready for synchronous read/write in Connection.
int WaitForEvent(int fd, int mask, long timeout);

// Event loop
class EventLoop {
 public:
  static EventLoop* Create();
  void Run();
  EventLoopStatus CreateFileEvent(int fd, FileEvent* file_event);
  EventLoopStatus DeleteFileEvent(int fd, int mask);
  void CreateTimeEvent(TimeEvent* time_event);
  void ProcessEvents();
  ~EventLoop();

 private:
  static constexpr const int kEventSize = 1024;
  explicit EventLoop(KqueueEventApi* kq);
  void ProcessFileEvents();
  void ProcessTimeEvents() const;
  std::vector<FileEvent*> file_events_;
  mutable TimeEvent* time_event_head_;
  const KqueueEventApi* event_api_;
  mutable int max_fd_;
};
}  // namespace ae
}  // namespace redis_simple
