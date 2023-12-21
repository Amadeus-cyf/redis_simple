#pragma once

#include <stdlib.h>

#include <unordered_map>
#include <vector>

namespace redis_simple {
namespace ae {
class AeKqueue {
 public:
  static AeKqueue* AeApiCreate(int nevents);
  int AeApiAddEvent(int fd, int mask) const;
  int AeApiDelEvent(int fd, int mask) const;
  std::unordered_map<int, int> AeApiPoll(struct timespec* tspec) const;
  ~AeKqueue();

 private:
  explicit AeKqueue(int fd, int nevents);
  int kqueue_fd_;
  int nevents_;
};
}  // namespace ae
}  // namespace redis_simple
