#pragma once

#include <sys/event.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "event_loop/ae_event_poller.h"

namespace redis_simple::ae {
class KqueuePoller final : public EventPoller {
 public:
  static std::unique_ptr<KqueuePoller> Create(int nevents);
  int AddEvent(int fd, int mask) const override;
  int DeleteEvent(int fd, int mask) const override;
  std::unordered_map<int, int> Poll(
      struct timespec* timeout_spec) const override;
  ~KqueuePoller() override;

 private:
  explicit KqueuePoller(int fd, int nevents);
  int kqueue_fd_;
  int nevents_;
  mutable std::vector<struct kevent> events_;
};
}  // namespace redis_simple::ae
