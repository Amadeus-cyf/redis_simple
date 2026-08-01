#include "server/shutdown.h"

#include <gtest/gtest.h>

#include <csignal>

namespace redis_simple::shutdown {
TEST(ShutdownTest, RecordsTerminationSignal) {
  ASSERT_TRUE(InstallSignalHandlers());
  ASSERT_EQ(std::raise(SIGTERM), 0);
  EXPECT_TRUE(StopRequested());
  EXPECT_TRUE(InstallSignalHandlers());
  EXPECT_FALSE(StopRequested());
}
}  // namespace redis_simple::shutdown
