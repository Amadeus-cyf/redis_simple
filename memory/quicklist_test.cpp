#include "memory/quicklist.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

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

TEST(QuickListTest, SplitTailOnLimit) {
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

TEST(QuickListTest, SplitHeadOnLimit) {
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

TEST(QuickListTest, AllowLargeSingleEntry) {
  QuickList quicklist(16);
  const std::string value(128, 'z');

  ASSERT_TRUE(quicklist.RPush(value));

  EXPECT_EQ(quicklist.Size(), 1);
  EXPECT_EQ(quicklist.NodeCount(), 1);
  EXPECT_EQ(quicklist.RPop(), value);
  EXPECT_TRUE(quicklist.Empty());
}

TEST(QuickListTest, MergeAfterLPop) {
  QuickList quicklist(96);
  const std::string value(32, 'm');

  ASSERT_TRUE(quicklist.RPush(value + "1"));
  ASSERT_TRUE(quicklist.RPush(value + "2"));
  ASSERT_TRUE(quicklist.RPush(value + "3"));
  ASSERT_EQ(quicklist.NodeCount(), 2);

  EXPECT_EQ(quicklist.LPop(), value + "1");

  EXPECT_EQ(quicklist.NodeCount(), 1);
  EXPECT_EQ(quicklist.Range(0, 1),
            (std::vector<std::string>{value + "2", value + "3"}));
}

TEST(QuickListTest, MergeAfterRPop) {
  QuickList quicklist(96);
  const std::string value(32, 'n');

  ASSERT_TRUE(quicklist.LPush(value + "3"));
  ASSERT_TRUE(quicklist.LPush(value + "2"));
  ASSERT_TRUE(quicklist.LPush(value + "1"));
  ASSERT_EQ(quicklist.NodeCount(), 2);

  EXPECT_EQ(quicklist.RPop(), value + "3");

  EXPECT_EQ(quicklist.NodeCount(), 1);
  EXPECT_EQ(quicklist.Range(0, 1),
            (std::vector<std::string>{value + "1", value + "2"}));
}

TEST(QuickListTest, RangeAcrossNodes) {
  QuickList quicklist(48);
  const std::string value(32, 'r');

  ASSERT_TRUE(quicklist.RPush(value + "1"));
  ASSERT_TRUE(quicklist.RPush(value + "2"));
  ASSERT_TRUE(quicklist.RPush(value + "3"));
  ASSERT_TRUE(quicklist.RPush(value + "4"));
  ASSERT_EQ(quicklist.NodeCount(), 4);

  EXPECT_EQ(quicklist.Range(0, 3),
            (std::vector<std::string>{value + "1", value + "2", value + "3",
                                      value + "4"}));
  EXPECT_EQ(quicklist.Range(1, 2),
            (std::vector<std::string>{value + "2", value + "3"}));
  EXPECT_EQ(quicklist.Range(2, 100),
            (std::vector<std::string>{value + "3", value + "4"}));
}

TEST(QuickListTest, RangeInvalidBounds) {
  QuickList quicklist(48);

  EXPECT_TRUE(quicklist.Range(0, 0).empty());

  ASSERT_TRUE(quicklist.RPush("one"));
  ASSERT_TRUE(quicklist.RPush("two"));

  EXPECT_TRUE(quicklist.Range(2, 3).empty());
  EXPECT_TRUE(quicklist.Range(1, 0).empty());
}
}  // namespace redis_simple::in_memory
