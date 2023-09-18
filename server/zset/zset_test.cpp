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

TEST_F(ZSetTest, Update) {
  zset->addOrUpdate("key1", 1.0);
  ASSERT_EQ(zset->size(), 4);

  zset->addOrUpdate("key3", 4.0);
  ASSERT_EQ(zset->size(), 4);

  int r1 = zset->getRank("key1");
  ASSERT_EQ(r1, 0);

  int r2 = zset->getRank("key2");
  ASSERT_EQ(r2, 2);

  int r3 = zset->getRank("key3");
  ASSERT_EQ(r3, 3);

  int r4 = zset->getRank("key4");
  ASSERT_EQ(r4, 1);

  zset->addOrUpdate("key1", 5.0);
  ASSERT_EQ(zset->size(), 4);

  r1 = zset->getRank("key1");
  ASSERT_EQ(r1, 3);

  r4 = zset->getRank("key4");
  ASSERT_EQ(r4, 0);
}

TEST_F(ZSetTest, Range) {
  const std::vector<const ZSet::ZSetEntry*>& r1 = zset->range(0, 3);
  ASSERT_EQ(r1.size(), 4);
  ASSERT_EQ(r1[0]->key, "key4");
  ASSERT_EQ(r1[0]->score, 1.0);
  ASSERT_EQ(r1[1]->key, "key2");
  ASSERT_EQ(r1[1]->score, 2.0);
  ASSERT_EQ(r1[2]->key, "key3");
  ASSERT_EQ(r1[2]->score, 4.0);
  ASSERT_EQ(r1[3]->key, "key1");
  ASSERT_EQ(r1[3]->score, 5.0);

  const std::vector<const ZSet::ZSetEntry*>& r2 = zset->range(1, 2);
  ASSERT_EQ(r2.size(), 2);
  ASSERT_EQ(r2[0]->key, "key2");
  ASSERT_EQ(r2[0]->score, 2.0);
  ASSERT_EQ(r2[1]->key, "key3");
  ASSERT_EQ(r2[1]->score, 4.0);

  const std::vector<const ZSet::ZSetEntry*>& r3 = zset->range(1, -1);
  ASSERT_EQ(r3.size(), 3);
  ASSERT_EQ(r3[0]->key, "key2");
  ASSERT_EQ(r3[0]->score, 2.0);
  ASSERT_EQ(r3[1]->key, "key3");
  ASSERT_EQ(r3[1]->score, 4.0);
  ASSERT_EQ(r3[2]->key, "key1");
  ASSERT_EQ(r3[2]->score, 5.0);

  const std::vector<const ZSet::ZSetEntry*>& r4 = zset->range(-2, -1);
  ASSERT_EQ(r4.size(), 2);
  ASSERT_EQ(r4[0]->key, "key3");
  ASSERT_EQ(r4[0]->score, 4.0);
  ASSERT_EQ(r4[1]->key, "key1");
  ASSERT_EQ(r4[1]->score, 5.0);

  const std::vector<const ZSet::ZSetEntry*>& r5 = zset->range(0, INT_MAX);
  ASSERT_EQ(r5.size(), 4);
  ASSERT_EQ(r5[0]->key, "key4");
  ASSERT_EQ(r5[0]->score, 1.0);
  ASSERT_EQ(r5[1]->key, "key2");
  ASSERT_EQ(r5[1]->score, 2.0);
  ASSERT_EQ(r5[2]->key, "key3");
  ASSERT_EQ(r5[2]->score, 4.0);
  ASSERT_EQ(r5[3]->key, "key1");
  ASSERT_EQ(r5[3]->score, 5.0);

  const std::vector<const ZSet::ZSetEntry*>& r6 = zset->range(INT_MIN, 1);
  ASSERT_TRUE(r6.empty());

  const std::vector<const ZSet::ZSetEntry*>& r7 = zset->range(4, 3);
  ASSERT_TRUE(r7.empty());

  const std::vector<const ZSet::ZSetEntry*>& r8 = zset->range(-1, -2);
  ASSERT_TRUE(r8.empty());

  const std::vector<const ZSet::ZSetEntry*>& r9 = zset->range(-1, 2);
  ASSERT_TRUE(r9.empty());
}

TEST_F(ZSetTest, RevRange) {
  const std::vector<const ZSet::ZSetEntry*>& r1 = zset->revrange(0, 3);
  ASSERT_EQ(r1.size(), 4);
  ASSERT_EQ(r1[0]->key, "key1");
  ASSERT_EQ(r1[0]->score, 5.0);
  ASSERT_EQ(r1[1]->key, "key3");
  ASSERT_EQ(r1[1]->score, 4.0);
  ASSERT_EQ(r1[2]->key, "key2");
  ASSERT_EQ(r1[2]->score, 2.0);
  ASSERT_EQ(r1[3]->key, "key4");
  ASSERT_EQ(r1[3]->score, 1.0);

  const std::vector<const ZSet::ZSetEntry*>& r2 = zset->revrange(1, 2);
  ASSERT_EQ(r2.size(), 2);
  ASSERT_EQ(r2[0]->key, "key3");
  ASSERT_EQ(r2[0]->score, 4.0);
  ASSERT_EQ(r2[1]->key, "key2");
  ASSERT_EQ(r2[1]->score, 2.0);

  const std::vector<const ZSet::ZSetEntry*>& r3 = zset->revrange(1, -1);
  ASSERT_EQ(r3.size(), 3);
  ASSERT_EQ(r3[0]->key, "key3");
  ASSERT_EQ(r3[0]->score, 4.0);
  ASSERT_EQ(r3[1]->key, "key2");
  ASSERT_EQ(r3[1]->score, 2.0);
  ASSERT_EQ(r3[2]->key, "key4");
  ASSERT_EQ(r3[2]->score, 1.0);

  const std::vector<const ZSet::ZSetEntry*>& r4 = zset->revrange(-2, -1);
  ASSERT_EQ(r4.size(), 2);
  ASSERT_EQ(r4[0]->key, "key2");
  ASSERT_EQ(r4[0]->score, 2.0);
  ASSERT_EQ(r4[1]->key, "key4");
  ASSERT_EQ(r4[1]->score, 1.0);

  const std::vector<const ZSet::ZSetEntry*>& r5 = zset->revrange(0, INT_MAX);
  ASSERT_EQ(r5.size(), 4);
  ASSERT_EQ(r5[0]->key, "key1");
  ASSERT_EQ(r5[0]->score, 5.0);
  ASSERT_EQ(r5[1]->key, "key3");
  ASSERT_EQ(r5[1]->score, 4.0);
  ASSERT_EQ(r5[2]->key, "key2");
  ASSERT_EQ(r5[2]->score, 2.0);
  ASSERT_EQ(r5[3]->key, "key4");
  ASSERT_EQ(r5[3]->score, 1.0);

  const std::vector<const ZSet::ZSetEntry*>& r6 = zset->revrange(INT_MIN, 1);
  ASSERT_TRUE(r6.empty());

  const std::vector<const ZSet::ZSetEntry*>& r7 = zset->revrange(4, 3);
  ASSERT_TRUE(r7.empty());

  const std::vector<const ZSet::ZSetEntry*>& r8 = zset->revrange(-1, -2);
  ASSERT_TRUE(r8.empty());

  const std::vector<const ZSet::ZSetEntry*>& r9 = zset->revrange(-1, 2);
  ASSERT_TRUE(r9.empty());
}

TEST_F(ZSetTest, Remove) {
  bool r1 = zset->remove("key1");
  ASSERT_TRUE(r1);
  ASSERT_EQ(zset->size(), 3);

  int rank = zset->getRank("key1");
  ASSERT_EQ(rank, -1);

  bool r2 = zset->remove("key not exist");
  ASSERT_FALSE(r2);
}
}  // namespace zset
}  // namespace redis_simple
