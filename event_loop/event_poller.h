#pragma once

#include <ctime>
#include <unordered_map>

namespace redis_simple::event_loop {
class EventPoller {
 public:
  virtual int AddEvent(int fd, int mask) const = 0;
  virtual int DeleteEvent(int fd, int mask) const = 0;
  virtual std::unordered_map<int, int> Poll(
      struct timespec* timeout_spec) const = 0;
  virtual ~EventPoller() = default;
};
}  // namespace redis_simple::event_loop
