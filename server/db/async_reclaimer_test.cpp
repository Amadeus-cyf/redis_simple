#include "server/db/async_reclaimer.h"

#include <gtest/gtest.h>

#include <string>

namespace redis_simple::db {
TEST(AsyncReclaimerTest, ReclaimsObjectsOnWorkerThread) {
  AsyncReclaimer reclaimer;
  EXPECT_FALSE(reclaimer.Reclaim(nullptr));

  constexpr size_t kObjectCount = 32;
  for (size_t index = 0; index < kObjectCount; ++index) {
    ASSERT_TRUE(reclaimer.Reclaim(
        RedisObject::CreateWithString(std::string(4096, 'x'))));
  }

  reclaimer.WaitUntilIdle();
  EXPECT_EQ(reclaimer.PendingCount(), 0);
}
}  // namespace redis_simple::db
