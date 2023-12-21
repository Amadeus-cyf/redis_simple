#include "server/zset/zset.h"

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
  static const ZSet* zset;
};

const ZSet* ZSetTest::zset;

TEST_F(ZSetTest, Add) {
  zset->AddOrUpdate("key1", 3.0);
  ASSERT_EQ(zset->size(), 1);

  zset->AddOrUpdate("key2", 2.0);
  ASSERT_EQ(zset->size(), 2);

  zset->AddOrUpdate("key3", 1.0);
  ASSERT_EQ(zset->size(), 3);

  zset->AddOrUpdate("key4", 1.0);
  ASSERT_EQ(zset->size(), 4);
}

TEST_F(ZSetTest, GetRankOfKey) {
  int r1 = zset->GetRankOfKey("key1");
  ASSERT_EQ(r1, 3);

  int r2 = zset->GetRankOfKey("key2");
  ASSERT_EQ(r2, 2);

  int r3 = zset->GetRankOfKey("key3");
  ASSERT_EQ(r3, 0);

  int r4 = zset->GetRankOfKey("key4");
  ASSERT_EQ(r4, 1);

  int r5 = zset->GetRankOfKey("key_not_exist");
  ASSERT_EQ(r5, -1);
}

TEST_F(ZSetTest, Update) {
  zset->AddOrUpdate("key1", 1.0);
  ASSERT_EQ(zset->size(), 4);

  zset->AddOrUpdate("key3", 4.0);
  ASSERT_EQ(zset->size(), 4);

  int r1 = zset->GetRankOfKey("key1");
  ASSERT_EQ(r1, 0);

  int r2 = zset->GetRankOfKey("key2");
  ASSERT_EQ(r2, 2);

  int r3 = zset->GetRankOfKey("key3");
  ASSERT_EQ(r3, 3);

  int r4 = zset->GetRankOfKey("key4");
  ASSERT_EQ(r4, 1);

  zset->AddOrUpdate("key1", 5.0);
  ASSERT_EQ(zset->size(), 4);

  r1 = zset->GetRankOfKey("key1");
  ASSERT_EQ(r1, 3);

  r4 = zset->GetRankOfKey("key4");
  ASSERT_EQ(r4, 0);
}

TEST_F(ZSetTest, Remove) {
  bool r1 = zset->Remove("key1");
  ASSERT_TRUE(r1);
  ASSERT_EQ(zset->size(), 3);

  int rank = zset->GetRankOfKey("key1");
  ASSERT_EQ(rank, -1);

  bool r2 = zset->Remove("key not exist");
  ASSERT_FALSE(r2);
}
}  // namespace zset
}  // namespace redis_simple
