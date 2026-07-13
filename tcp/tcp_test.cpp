#include "tcp/tcp.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

namespace redis_simple::tcp {
TEST(TcpTest, CreatedSocketIsNonblockingAndCloseOnExec) {
  const int fd = TcpCreateSocket(AF_INET, true);
  ASSERT_GE(fd, 0);

  const int status_flags = fcntl(fd, F_GETFL);
  const int descriptor_flags = fcntl(fd, F_GETFD);
  EXPECT_NE(status_flags, -1);
  EXPECT_NE(status_flags & O_NONBLOCK, 0);
  EXPECT_NE(descriptor_flags, -1);
  EXPECT_NE(descriptor_flags & FD_CLOEXEC, 0);

  close(fd);
}
}  // namespace redis_simple::tcp
