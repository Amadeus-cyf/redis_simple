#pragma once

#include <sys/epoll.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "event_loop/ae_event_api.h"

namespace redis_simple::ae {
class EpollEventApi final : public EventApi {
 public:
  static std::unique_ptr<EpollEventApi> Create(int nevents);
  int AddEvent(int fd, int mask) const override;
  int DeleteEvent(int fd, int mask) const override;
  std::unordered_map<int, int> Poll(
      struct timespec* timeout_spec) const override;
  ~EpollEventApi() override;

 private:
  explicit EpollEventApi(int fd, int nevents);
  int epoll_fd_;
  int nevents_;
  mutable std::vector<struct epoll_event> events_;
  mutable std::unordered_map<int, int> masks_;
};
}  // namespace redis_simple::ae
