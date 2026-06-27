#pragma once

#include <sys/event.h>

#include <unordered_map>
#include <vector>

namespace redis_simple {
namespace ae {
class KqueueEventApi {
 public:
  static KqueueEventApi* Create(int nevents);
  int AddEvent(int fd, int mask) const;
  int DeleteEvent(int fd, int mask) const;
  std::unordered_map<int, int> Poll(struct timespec* timeout_spec) const;
  ~KqueueEventApi();

 private:
  explicit KqueueEventApi(int fd, int nevents);
  int kqueue_fd_;
  int nevents_;
  mutable std::vector<struct kevent> events_;
};
}  // namespace ae
}  // namespace redis_simple
