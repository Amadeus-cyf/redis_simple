#include "memory/quicklist.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <string_view>
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

TEST(QuickListTest, SetSplitsOversizedNode) {
  QuickList quicklist(96);
  const std::string value(32, 's');
  const std::string large_value(128, 'l');

  ASSERT_TRUE(quicklist.RPush(value + "1"));
  ASSERT_TRUE(quicklist.RPush(value + "2"));
  ASSERT_EQ(quicklist.NodeCount(), 1);

  ASSERT_TRUE(quicklist.Set(0, large_value));

  EXPECT_EQ(quicklist.NodeCount(), 2);
  EXPECT_EQ(quicklist.Range(0, 1),
            (std::vector<std::string>{large_value, value + "2"}));
  EXPECT_FALSE(quicklist.Set(2, "missing"));
}

TEST(QuickListTest, RemoveAcrossNodes) {
  QuickList quicklist(48);
  const std::string value(32, 'd');

  ASSERT_TRUE(quicklist.RPush(value + "1"));
  ASSERT_TRUE(quicklist.RPush(value + "2"));
  ASSERT_TRUE(quicklist.RPush(value + "3"));
  ASSERT_TRUE(quicklist.RPush(value + "2"));
  ASSERT_TRUE(quicklist.RPush(value + "4"));
  ASSERT_EQ(quicklist.NodeCount(), 5);

  ASSERT_EQ(
      quicklist.Remove(value + "2", 1, QuickList::RemoveDirection::kFromTail),
      1);
  EXPECT_EQ(quicklist.Range(0, 3),
            (std::vector<std::string>{value + "1", value + "2", value + "3",
                                      value + "4"}));

  ASSERT_EQ(
      quicklist.Remove(value + "2", 0, QuickList::RemoveDirection::kFromHead),
      1);
  EXPECT_EQ(quicklist.Range(0, 2),
            (std::vector<std::string>{value + "1", value + "3", value + "4"}));
}

TEST(QuickListTest, TrimAcrossNodes) {
  QuickList quicklist(48);
  const std::string value(32, 't');

  ASSERT_TRUE(quicklist.RPush(value + "1"));
  ASSERT_TRUE(quicklist.RPush(value + "2"));
  ASSERT_TRUE(quicklist.RPush(value + "3"));
  ASSERT_TRUE(quicklist.RPush(value + "4"));
  ASSERT_TRUE(quicklist.RPush(value + "5"));
  ASSERT_EQ(quicklist.NodeCount(), 5);

  ASSERT_TRUE(quicklist.Trim(1, 3));
  EXPECT_EQ(quicklist.Range(0, 2),
            (std::vector<std::string>{value + "2", value + "3", value + "4"}));

  ASSERT_TRUE(quicklist.Trim(1, 0));
  EXPECT_TRUE(quicklist.Empty());
  EXPECT_EQ(quicklist.NodeCount(), 0);
}

TEST(QuickListTest, RemoveFromTailWithinOneNode) {
  QuickList quicklist(256);
  for (std::string_view value : {"x", "keep", "x", "x", "keep", "x"}) {
    ASSERT_TRUE(quicklist.RPush(value));
  }
  ASSERT_EQ(quicklist.NodeCount(), 1);

  EXPECT_EQ(quicklist.Remove("x", 2, QuickList::RemoveDirection::kFromTail), 2);
  EXPECT_EQ(quicklist.Range(0, quicklist.Size() - 1),
            (std::vector<std::string>{"x", "keep", "x", "keep"}));
}

TEST(QuickListTest, ReleasesSingleListPackWithoutRebuilding) {
  QuickList quicklist(256);
  ASSERT_TRUE(quicklist.RPush("one"));
  ASSERT_TRUE(quicklist.RPush("two"));
  ASSERT_EQ(quicklist.NodeCount(), 1);

  auto listpack = quicklist.ReleaseListPack();

  ASSERT_NE(listpack, nullptr);
  EXPECT_TRUE(quicklist.Empty());
  EXPECT_EQ(quicklist.NodeCount(), 0);
  ASSERT_EQ(listpack->Size(), 2);
  const auto first = listpack->First();
  const auto last = listpack->Last();
  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(last.has_value());
  EXPECT_EQ(listpack->Get(first.value_or(0)), "one");
  EXPECT_EQ(listpack->Get(last.value_or(0)), "two");
}
}  // namespace redis_simple::in_memory
