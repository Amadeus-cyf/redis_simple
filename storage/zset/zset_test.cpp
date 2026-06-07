#include "storage/zset/zset.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"

namespace redis_simple {
namespace zset {
class ZSetTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { zset = std::unique_ptr<ZSet>(ZSet::Init()); }
  static void TearDownTestSuite() { zset.reset(); }

  static std::unique_ptr<ZSet> zset;
};

std::unique_ptr<ZSet> ZSetTest::zset = nullptr;
TEST_F(ZSetTest, InsertAndConvert) {
  for (int i = 0; i < 256; ++i) {
    std::string prefix("key_");
    prefix.append(std::to_string(i));
    ASSERT_TRUE(zset->InsertOrUpdate(prefix, i));
  }
  ASSERT_EQ(zset->Size(), 256);
  ASSERT_EQ(zset->GetRankOfKey("key_0"), 0);
  ASSERT_EQ(zset->GetRankOfKey("key_255"), 255);
  ASSERT_EQ(zset->GetRankOfKey("key_256").value_or(-1), -1);

  RangeByRankSpec spec;
  spec.min = 0;
  spec.max = 255;
  spec.minex = false;
  spec.maxex = false;
  spec.limit = std::make_unique<LimitSpec>(0, -1);
  spec.reverse = false;
  const auto entries = zset->RangeByRank(&spec);
  ASSERT_EQ(entries.size(), 256);
  ASSERT_EQ(entries.front()->key, "key_0");
  ASSERT_EQ(entries.back()->key, "key_255");
}

TEST(ZSetEncodingTest, ConvertsToSkiplistWhenMemberIsLong) {
  auto zset = std::unique_ptr<ZSet>(ZSet::Init());
  ASSERT_TRUE(zset->InsertOrUpdate("short", 1.0));
  ASSERT_EQ(zset->GetEncoding(), ZSet::Encoding::kListPack);

  ASSERT_TRUE(zset->InsertOrUpdate(std::string(65, 'x'), 2.0));

  ASSERT_EQ(zset->GetEncoding(), ZSet::Encoding::kSkiplist);
  ASSERT_EQ(zset->Size(), 2);
  ASSERT_EQ(zset->GetScoreOfKey("short"), 1.0);
}

TEST(ZSetEncodingTest, ConvertsToSkiplistWhenEntryCountExceedsLimit) {
  auto zset = std::unique_ptr<ZSet>(ZSet::Init());
  for (int i = 0; i < 128; ++i) {
    ASSERT_TRUE(zset->InsertOrUpdate("key_" + std::to_string(i), i));
  }
  ASSERT_EQ(zset->GetEncoding(), ZSet::Encoding::kListPack);

  ASSERT_TRUE(zset->InsertOrUpdate("key_128", 128.0));

  ASSERT_EQ(zset->GetEncoding(), ZSet::Encoding::kSkiplist);
  ASSERT_EQ(zset->Size(), 129);
  ASSERT_EQ(zset->GetRankOfKey("key_0"), 0);
  ASSERT_EQ(zset->GetRankOfKey("key_128"), 128);
}
}  // namespace zset
}  // namespace redis_simple
