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
using KeyScorePair = std::pair<std::string, double>;
std::vector<std::pair<std::string, double>> ToKeyScorePairs(
    const std::vector<const ZSet::ZSetEntry*> keys);

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

TEST_F(ZSetTest, RangeByRank) {
  /* base */
  const ZSet::RangeByRankSpec& spec0 = {
      .min = 0,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<const ZSet::ZSetEntry*>& k0 = zset->RangeByRank(&spec0);
  const std::vector<KeyScorePair>& p0 = ToKeyScorePairs(k0);
  const std::vector<KeyScorePair>& e0 = {
      {"key4", 1.0}, {"key2", 2.0}, {"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p0, e0);

  /* min exclusive */
  const ZSet::RangeByRankSpec& spec1 = {
      .min = 1,
      .max = 3,
      .minex = true,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<const ZSet::ZSetEntry*>& k1 = zset->RangeByRank(&spec1);
  const std::vector<KeyScorePair>& p1 = ToKeyScorePairs(k1);
  const std::vector<KeyScorePair>& e1 = {{"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p1, e1);

  /* max exclusive */
  const ZSet::RangeByRankSpec& spec2 = {
      .min = 1,
      .max = 3,
      .minex = false,
      .maxex = true,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<const ZSet::ZSetEntry*>& k2 = zset->RangeByRank(&spec2);
  const std::vector<KeyScorePair>& p2 = ToKeyScorePairs(k2);
  const std::vector<KeyScorePair>& e2 = {{"key2", 2.0}, {"key3", 4.0}};
  ASSERT_EQ(p2, e2);

  /* min and max exclusive */
  const ZSet::RangeByRankSpec& spec3 = {
      .min = 1,
      .max = 3,
      .minex = true,
      .maxex = true,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<const ZSet::ZSetEntry*>& k3 = zset->RangeByRank(&spec3);
  const std::vector<KeyScorePair>& p3 = ToKeyScorePairs(k3);
  const std::vector<KeyScorePair>& e3 = {{"key3", 4.0}};
  ASSERT_EQ(p3, e3);

  /* limit */
  const ZSet::RangeByRankSpec& spec4 = {
      .min = 0,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(2, 3),
      .reverse = false,
  };
  const std::vector<const ZSet::ZSetEntry*>& k4 = zset->RangeByRank(&spec4);
  const std::vector<KeyScorePair>& p4 = ToKeyScorePairs(k4);
  const std::vector<KeyScorePair>& e4 = {{"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p4, e4);

  /* reverse */
  const ZSet::RangeByRankSpec& spec5 = {
      .min = 0,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = true,
  };
  const std::vector<const ZSet::ZSetEntry*>& k5 = zset->RangeByRank(&spec5);
  const std::vector<KeyScorePair>& p5 = ToKeyScorePairs(k5);
  const std::vector<KeyScorePair>& e5 = {
      {"key1", 5.0}, {"key3", 4.0}, {"key2", 2.0}, {"key4", 1.0}};
  ASSERT_EQ(p5, e5);

  /* reverse with offset */
  const ZSet::RangeByRankSpec& spec6 = {
      .min = 0,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(1, 2),
      .reverse = true,
  };
  const std::vector<const ZSet::ZSetEntry*>& k6 = zset->RangeByRank(&spec6);
  const std::vector<KeyScorePair>& p6 = ToKeyScorePairs(k6);
  const std::vector<KeyScorePair>& e6 = {{"key3", 4.0}, {"key2", 2.0}};
  ASSERT_EQ(p6, e6);

  /* count = 0 */
  const ZSet::RangeByRankSpec& spec7 = {
      .min = 0,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, 0),
      .reverse = false,
  };
  const std::vector<const ZSet::ZSetEntry*>& k7 = zset->RangeByRank(&spec7);
  const std::vector<KeyScorePair>& p7 = ToKeyScorePairs(k7);
  ASSERT_EQ(p7.size(), 0);

  /* invalid spec non-exclusive, min > max */
  const ZSet::RangeByRankSpec& spec8 = {
      .min = 2,
      .max = 1,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<const ZSet::ZSetEntry*>& k8 = zset->RangeByRank(&spec8);
  const std::vector<KeyScorePair>& p8 = ToKeyScorePairs(k8);
  ASSERT_EQ(p8.size(), 0);

  /* invalid spec exclusive, min >= max */
  const ZSet::RangeByRankSpec& spec9 = {
      .min = 1,
      .max = 1,
      .minex = false,
      .maxex = true,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<const ZSet::ZSetEntry*>& k9 = zset->RangeByRank(&spec9);
  const std::vector<KeyScorePair>& p9 = ToKeyScorePairs(k9);
  ASSERT_EQ(p9.size(), 0);

  /* min out of range */
  const ZSet::RangeByRankSpec& spec10 = {
      .min = 10,
      .max = 12,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<const ZSet::ZSetEntry*>& k10 = zset->RangeByRank(&spec10);
  const std::vector<KeyScorePair>& p10 = ToKeyScorePairs(k10);
  ASSERT_EQ(p10.size(), 0);

  /* offset out of range */
  const ZSet::RangeByRankSpec& spec11 = {
      .min = 0,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(10, -1),
      .reverse = false,
  };
  const std::vector<const ZSet::ZSetEntry*>& k11 = zset->RangeByRank(&spec11);
  const std::vector<KeyScorePair>& p11 = ToKeyScorePairs(k11);
  ASSERT_EQ(p11.size(), 0);
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

std::vector<KeyScorePair> ToKeyScorePairs(
    const std::vector<const ZSet::ZSetEntry*> keys) {
  std::vector<std::pair<std::string, double>> pairs;
  for (int i = 0; i < keys.size(); ++i) {
    pairs.push_back({keys[i]->key, keys[i]->score});
  }
  return pairs;
}
}  // namespace zset
}  // namespace redis_simple
