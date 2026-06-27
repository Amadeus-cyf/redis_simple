#pragma once

#include "ae_file_event.h"
#include "ae_kqueue.h"
#include "ae_time_event.h"

namespace redis_simple::ae {
enum class EventFlag {
  kReadable = 1,
  kWritable = 1 << 1,
  kBarrier = 1 << 2,
  kNoMore = -1,
  kDeleteEventId = -1024,
};

constexpr int ToInt(EventFlag flag) { return static_cast<int>(flag); }
inline int& operator|=(int& mask, EventFlag flag) {
  mask |= ToInt(flag);
  return mask;
}
inline int& operator&=(int& mask, EventFlag flag) {
  mask &= ToInt(flag);
  return mask;
}
constexpr int operator&(int mask, EventFlag flag) { return mask & ToInt(flag); }
constexpr int operator&(EventFlag flag, int mask) { return ToInt(flag) & mask; }
constexpr int operator~(EventFlag flag) { return ~ToInt(flag); }
constexpr bool operator==(int value, EventFlag flag) {
  return value == ToInt(flag);
}
constexpr bool operator!=(int value, EventFlag flag) {
  return value != ToInt(flag);
}
constexpr bool operator==(EventFlag flag, int value) { return value == flag; }
constexpr bool operator!=(EventFlag flag, int value) { return value != flag; }

enum class EventLoopStatus {
  kOk = 0,
  kError = -1,
};

enum class EventCallbackStatus {
  kOk = 0,
  kError = -1,
};

// Return the readiness mask for fd, or 0 on timeout.
int WaitForEvent(int fd, int mask, long timeout);
inline int WaitForEvent(int fd, EventFlag mask, long timeout) {
  return WaitForEvent(fd, ToInt(mask), timeout);
}

class EventLoop {
 public:
  static EventLoop* Create();
  void Run();
  EventLoopStatus CreateFileEvent(int fd, FileEvent* file_event);
  EventLoopStatus DeleteFileEvent(int fd, int mask);
  EventLoopStatus DeleteFileEvent(int fd, EventFlag mask) {
    return DeleteFileEvent(fd, ToInt(mask));
  }
  void CreateTimeEvent(TimeEvent* time_event);
  void ProcessEvents();
  ~EventLoop();

 private:
  static constexpr int kEventSize = 1024;
  explicit EventLoop(KqueueEventApi* kq);
  void ProcessFileEvents();
  void ProcessTimeEvents() const;
  std::vector<FileEvent*> file_events_;
  mutable TimeEvent* time_event_head_;
  const KqueueEventApi* event_api_;
  mutable int max_fd_;
};
}  // namespace redis_simple::ae
