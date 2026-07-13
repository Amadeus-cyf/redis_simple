#pragma once

#include <ctime>
#include <vector>

namespace redis_simple::event_loop {
struct ReadyEvent {
  int fd;
  int mask;
};

class EventPoller {
 public:
  virtual int AddEvent(int fd, int mask) = 0;
  virtual int DeleteEvent(int fd, int mask) = 0;
  virtual const std::vector<ReadyEvent>& Poll(
      struct timespec* timeout_spec) = 0;
  virtual ~EventPoller() = default;
};
}  // namespace redis_simple::event_loop
