#include "server/zset/zset.h"

#include "gtest/gtest.h"

namespace redis_simple {
namespace zset {
class ZSetTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { zset = ZSet::init(); }
  static void TearDownTestSuit() {
    delete zset;
    zset = nullptr;
  }
  static const ZSet* zset;
};

const ZSet* ZSetTest::zset;

TEST_F(ZSetTest, Add) {
  zset->addOrUpdate("key1", 3.0);
  ASSERT_EQ(zset->size(), 1);

  zset->addOrUpdate("key2", 2.0);
  ASSERT_EQ(zset->size(), 2);

  zset->addOrUpdate("key3", 1.0);
  ASSERT_EQ(zset->size(), 3);

  zset->addOrUpdate("key4", 1.0);
  ASSERT_EQ(zset->size(), 4);
}

TEST_F(ZSetTest, GetRank) {
  int r1 = zset->getRank("key1");
  ASSERT_EQ(r1, 3);

  int r2 = zset->getRank("key2");
  ASSERT_EQ(r2, 2);

  int r3 = zset->getRank("key3");
  ASSERT_EQ(r3, 0);

  int r4 = zset->getRank("key4");
  ASSERT_EQ(r4, 1);

  int r5 = zset->getRank("key_not_exist");
  ASSERT_EQ(r5, -1);
}
}  // namespace zset
}  // namespace redis_simple
