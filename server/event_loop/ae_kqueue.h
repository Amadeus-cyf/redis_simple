#pragma once

#include <stdlib.h>

#include <unordered_map>
#include <vector>

namespace redis_simple {
namespace ae {
class AeKqueue {
 public:
  static AeKqueue* aeApiCreate(int nevents);
  int aeApiAddEvent(int fd, int mask);
  int aeApiDelEvent(int fd, int mask);
  std::unordered_map<int, int> aeApiPoll(struct timespec* tspec);
  ~AeKqueue();

 private:
  explicit AeKqueue(int fd, int nevents);
  int kqueue_fd;
  int nevents;
};
}  // namespace ae
}  // namespace redis_simple
