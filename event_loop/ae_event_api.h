#pragma once

#include <ctime>
#include <unordered_map>

namespace redis_simple::ae {
class EventApi {
 public:
  virtual int AddEvent(int fd, int mask) const = 0;
  virtual int DeleteEvent(int fd, int mask) const = 0;
  virtual std::unordered_map<int, int> Poll(
      struct timespec* timeout_spec) const = 0;
  virtual ~EventApi() = default;
};
}  // namespace redis_simple::ae
