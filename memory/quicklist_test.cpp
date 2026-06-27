#include "memory/quicklist.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>

namespace redis_simple::in_memory {
TEST(QuickListTest, PushAndPopPreserveDequeOrder) {
  QuickList quicklist;

  ASSERT_TRUE(quicklist.LPush("b"));
  ASSERT_TRUE(quicklist.LPush("a"));
  ASSERT_TRUE(quicklist.RPush("c"));
  ASSERT_TRUE(quicklist.RPush("d"));

  EXPECT_EQ(quicklist.Size(), 4);
  EXPECT_EQ(quicklist.NodeCount(), 1);
  EXPECT_EQ(quicklist.LPop(), "a");
  EXPECT_EQ(quicklist.RPop(), "d");
  EXPECT_EQ(quicklist.LPop(), "b");
  EXPECT_EQ(quicklist.RPop(), "c");
  EXPECT_EQ(quicklist.LPop(), std::nullopt);
  EXPECT_TRUE(quicklist.Empty());
  EXPECT_EQ(quicklist.NodeCount(), 0);
}

TEST(QuickListTest, SplitsTailWhenListpackReachesByteLimit) {
  QuickList quicklist(48);
  const std::string value(32, 'x');

  ASSERT_TRUE(quicklist.RPush(value + "1"));
  ASSERT_TRUE(quicklist.RPush(value + "2"));
  ASSERT_TRUE(quicklist.RPush(value + "3"));

  EXPECT_EQ(quicklist.Size(), 3);
  EXPECT_EQ(quicklist.NodeCount(), 3);
  EXPECT_EQ(quicklist.LPop(), value + "1");
  EXPECT_EQ(quicklist.LPop(), value + "2");
  EXPECT_EQ(quicklist.LPop(), value + "3");
  EXPECT_EQ(quicklist.NodeCount(), 0);
}

TEST(QuickListTest, SplitsHeadWhenListpackReachesByteLimit) {
  QuickList quicklist(48);
  const std::string value(32, 'y');

  ASSERT_TRUE(quicklist.LPush(value + "1"));
  ASSERT_TRUE(quicklist.LPush(value + "2"));
  ASSERT_TRUE(quicklist.LPush(value + "3"));

  EXPECT_EQ(quicklist.Size(), 3);
  EXPECT_EQ(quicklist.NodeCount(), 3);
  EXPECT_EQ(quicklist.RPop(), value + "1");
  EXPECT_EQ(quicklist.RPop(), value + "2");
  EXPECT_EQ(quicklist.RPop(), value + "3");
  EXPECT_EQ(quicklist.NodeCount(), 0);
}

TEST(QuickListTest, AllowsSingleEntryLargerThanNodeLimit) {
  QuickList quicklist(16);
  const std::string value(128, 'z');

  ASSERT_TRUE(quicklist.RPush(value));

  EXPECT_EQ(quicklist.Size(), 1);
  EXPECT_EQ(quicklist.NodeCount(), 1);
  EXPECT_EQ(quicklist.RPop(), value);
  EXPECT_TRUE(quicklist.Empty());
}
}  // namespace redis_simple::in_memory
