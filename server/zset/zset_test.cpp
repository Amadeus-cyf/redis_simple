#include "server/zset/zset.h"

#include <string>

#include "gtest/gtest.h"

namespace redis_simple {
namespace zset {
class ZSetTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { zset = ZSet::Init(); }
  static void TearDownTestSuit() {
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
}
}  // namespace zset
}  // namespace redis_simple
