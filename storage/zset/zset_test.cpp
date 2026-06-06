#include "storage/zset/zset.h"

#include <string>

#include "gtest/gtest.h"

namespace redis_simple {
namespace zset {
class ZSetTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { zset = ZSet::Init(); }
  static void TearDownTestSuite() {
    delete zset;
    zset = nullptr;
  }
  static ZSet* zset;
};

ZSet* ZSetTest::zset;
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
}  // namespace zset
}  // namespace redis_simple
