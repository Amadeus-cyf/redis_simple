#include "server/zset/zset.h"

#include <limits>

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
const std::vector<std::pair<std::string, double>> ToKeyScorePairs(
    const std::vector<const ZSet::ZSetEntry*>& keys);

TEST_F(ZSetTest, Add) {
  zset->AddOrUpdate("key1", 3.0);
  ASSERT_EQ(zset->Size(), 1);

  zset->AddOrUpdate("key2", 2.0);
  ASSERT_EQ(zset->Size(), 2);

  zset->AddOrUpdate("key3", 1.0);
  ASSERT_EQ(zset->Size(), 3);

  zset->AddOrUpdate("key4", 1.0);
  ASSERT_EQ(zset->Size(), 4);
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
  ASSERT_EQ(zset->Size(), 4);

  zset->AddOrUpdate("key3", 4.0);
  ASSERT_EQ(zset->Size(), 4);

  int r1 = zset->GetRankOfKey("key1");
  ASSERT_EQ(r1, 0);

  int r2 = zset->GetRankOfKey("key2");
  ASSERT_EQ(r2, 2);

  int r3 = zset->GetRankOfKey("key3");
  ASSERT_EQ(r3, 3);

  int r4 = zset->GetRankOfKey("key4");
  ASSERT_EQ(r4, 1);

  zset->AddOrUpdate("key1", 5.0);
  ASSERT_EQ(zset->Size(), 4);

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
  const std::vector<KeyScorePair>& p0 =
      ToKeyScorePairs(zset->RangeByRank(&spec0));
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
  const std::vector<KeyScorePair>& p1 =
      ToKeyScorePairs(zset->RangeByRank(&spec1));
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
  const std::vector<KeyScorePair>& p3 =
      ToKeyScorePairs(zset->RangeByRank(&spec3));
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
  const std::vector<KeyScorePair>& p4 =
      ToKeyScorePairs(zset->RangeByRank(&spec4));
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
  const std::vector<KeyScorePair>& p5 =
      ToKeyScorePairs(zset->RangeByRank(&spec5));
  const std::vector<KeyScorePair>& e5 = {
      {"key1", 5.0}, {"key3", 4.0}, {"key2", 2.0}, {"key4", 1.0}};
  ASSERT_EQ(p5, e5);

  /* reverse with limit */
  const ZSet::RangeByRankSpec& spec6 = {
      .min = 0,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(1, 2),
      .reverse = true,
  };
  const std::vector<KeyScorePair>& p6 =
      ToKeyScorePairs(zset->RangeByRank(&spec6));
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
  const std::vector<KeyScorePair>& p7 =
      ToKeyScorePairs(zset->RangeByRank(&spec7));
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
  const std::vector<KeyScorePair>& p9 =
      ToKeyScorePairs(zset->RangeByRank(&spec9));
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
  const std::vector<KeyScorePair>& p10 =
      ToKeyScorePairs(zset->RangeByRank(&spec10));
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
  const std::vector<KeyScorePair>& p11 =
      ToKeyScorePairs(zset->RangeByRank(&spec11));
  ASSERT_EQ(p11.size(), 0);

  /* negative index */
  const ZSet::RangeByRankSpec& spec12 = {
      .min = -3,
      .max = -1,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p12 =
      ToKeyScorePairs(zset->RangeByRank(&spec12));
  const std::vector<KeyScorePair>& e12 = {
      {"key2", 2.0}, {"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p12, e12);

  /* negative index, min exclusive */
  const ZSet::RangeByRankSpec& spec13 = {
      .min = -3,
      .max = -1,
      .minex = true,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p13 =
      ToKeyScorePairs(zset->RangeByRank(&spec13));
  const std::vector<KeyScorePair>& e13 = {{"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p13, e13);

  /* negative index, max exclusive */
  const ZSet::RangeByRankSpec& spec14 = {
      .min = -3,
      .max = -1,
      .minex = false,
      .maxex = true,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p14 =
      ToKeyScorePairs(zset->RangeByRank(&spec14));
  const std::vector<KeyScorePair>& e14 = {{"key2", 2.0}, {"key3", 4.0}};
  ASSERT_EQ(p14, e14);

  /* negative index, reverse */
  const ZSet::RangeByRankSpec& spec15 = {
      .min = -3,
      .max = -1,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = true,
  };
  const std::vector<KeyScorePair>& p15 =
      ToKeyScorePairs(zset->RangeByRank(&spec15));
  const std::vector<KeyScorePair>& e15 = {
      {"key3", 4.0}, {"key2", 2.0}, {"key4", 1.0}};
  ASSERT_EQ(p15, e15);

  /* negative index, invalid spec non-exclusive, min > max */
  const ZSet::RangeByRankSpec& spec16 = {
      .min = -1,
      .max = -3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p16 =
      ToKeyScorePairs(zset->RangeByRank(&spec16));
  ASSERT_EQ(p16.size(), 0);

  /* negative index, invalid spec exclusive, min >= max */
  const ZSet::RangeByRankSpec& spec17 = {
      .min = -1,
      .max = -1,
      .minex = true,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p17 =
      ToKeyScorePairs(zset->RangeByRank(&spec17));
  ASSERT_EQ(p17.size(), 0);

  /* negative index, invalid spec, min out of range */
  const ZSet::RangeByRankSpec& spec18 = {
      .min = -10,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p18 =
      ToKeyScorePairs(zset->RangeByRank(&spec18));
  ASSERT_EQ(p18.size(), 0);

  /* negative index, invalid spec, max out of range */
  const ZSet::RangeByRankSpec& spec19 = {
      .min = -1,
      .max = -6,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p19 =
      ToKeyScorePairs(zset->RangeByRank(&spec19));
  ASSERT_EQ(p19.size(), 0);
}

TEST_F(ZSetTest, RangeByScore) {
  /* base */
  const ZSet::RangeByScoreSpec& spec0 = {
      .min = 1.0,
      .max = 5.0,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p0 =
      ToKeyScorePairs(zset->RangeByScore(&spec0));
  const std::vector<KeyScorePair>& e0 = {
      {"key4", 1.0}, {"key2", 2.0}, {"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p0, e0);

  /* infinity */
  const ZSet::RangeByScoreSpec& spec1 = {
      .min = -std::numeric_limits<double>::infinity(),
      .max = std::numeric_limits<double>::infinity(),
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p1 =
      ToKeyScorePairs(zset->RangeByScore(&spec1));
  const std::vector<KeyScorePair>& e1 = {
      {"key4", 1.0}, {"key2", 2.0}, {"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p1, e1);

  /* min exclusive */
  const ZSet::RangeByScoreSpec& spec2 = {
      .min = 2.0,
      .max = 5.0,
      .minex = true,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p2 =
      ToKeyScorePairs(zset->RangeByScore(&spec2));
  const std::vector<KeyScorePair>& e2 = {{"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p2, e2);

  /* max exclusive */
  const ZSet::RangeByScoreSpec& spec3 = {
      .min = 2.0,
      .max = 5.0,
      .minex = false,
      .maxex = true,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p3 =
      ToKeyScorePairs(zset->RangeByScore(&spec3));
  const std::vector<KeyScorePair>& e3 = {{"key2", 2.0}, {"key3", 4.0}};
  ASSERT_EQ(p3, e3);

  /* min and max exclusive */
  const ZSet::RangeByScoreSpec& spec4 = {
      .min = 2.0,
      .max = 5.0,
      .minex = true,
      .maxex = true,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p4 =
      ToKeyScorePairs(zset->RangeByScore(&spec4));
  const std::vector<KeyScorePair>& e4 = {{"key3", 4.0}};
  ASSERT_EQ(p4, e4);

  /* with limit */
  const ZSet::RangeByScoreSpec& spec5 = {
      .min = 1.0,
      .max = 6.0,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(1, 2),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p5 =
      ToKeyScorePairs(zset->RangeByScore(&spec5));
  const std::vector<KeyScorePair>& e5 = {{"key2", 2.0}, {"key3", 4.0}};
  ASSERT_EQ(p5, e5);

  /* reverse */
  const ZSet::RangeByScoreSpec& spec6 = {
      .min = 2.0,
      .max = 6.0,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = true,
  };
  const std::vector<KeyScorePair>& p6 =
      ToKeyScorePairs(zset->RangeByScore(&spec6));
  const std::vector<KeyScorePair>& e6 = {
      {"key1", 5.0}, {"key3", 4.0}, {"key2", 2.0}};
  ASSERT_EQ(p6, e6);

  /* reverse with limit */
  const ZSet::RangeByScoreSpec& spec7 = {
      .min = 1.0,
      .max = 6.0,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(1, 2),
      .reverse = true,
  };
  const std::vector<KeyScorePair>& p7 =
      ToKeyScorePairs(zset->RangeByScore(&spec7));
  const std::vector<KeyScorePair>& e7 = {{"key3", 4.0}, {"key2", 2.0}};
  ASSERT_EQ(p7, e7);

  /* count = 0 */
  const ZSet::RangeByScoreSpec& spec8 = {
      .min = 1.0,
      .max = 6.0,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, 0),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p8 =
      ToKeyScorePairs(zset->RangeByScore(&spec8));
  ASSERT_EQ(p8.size(), 0);

  /* invalid spec, non-exclusive, min >= max */
  const ZSet::RangeByScoreSpec& spec9 = {
      .min = 6.0,
      .max = 1.0,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p9 =
      ToKeyScorePairs(zset->RangeByScore(&spec9));
  ASSERT_EQ(p9.size(), 0);

  /* invalid spec, min exclusive, min >= max */
  const ZSet::RangeByScoreSpec& spec10 = {
      .min = 1.0,
      .max = 1.0,
      .minex = true,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p10 =
      ToKeyScorePairs(zset->RangeByScore(&spec10));
  ASSERT_EQ(p10.size(), 0);

  /* invalid spec, max exclusive, min >= max */
  const ZSet::RangeByScoreSpec& spec11 = {
      .min = 1.0,
      .max = 1.0,
      .minex = false,
      .maxex = true,
      .limit = std::make_unique<ZSet::LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p11 =
      ToKeyScorePairs(zset->RangeByScore(&spec11));
  ASSERT_EQ(p11.size(), 0);

  /* invalid spec, offset out of range */
  const ZSet::RangeByScoreSpec& spec12 = {
      .min = 1.0,
      .max = 6.0,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<ZSet::LimitSpec>(5, 0),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p12 =
      ToKeyScorePairs(zset->RangeByScore(&spec12));
  ASSERT_EQ(p12.size(), 0);
}

TEST_F(ZSetTest, Count) {
  /* base */
  const ZSet::RangeByScoreSpec& spec1 = {
      .min = 1.0,
      .max = 6.0,
      .minex = false,
      .maxex = false,
  };
  const size_t c1 = zset->Count(&spec1);
  ASSERT_EQ(c1, 4);

  /* min exclusive */
  const ZSet::RangeByScoreSpec& spec2 = {
      .min = 2.0,
      .max = 5.0,
      .minex = true,
      .maxex = false,
  };
  const size_t c2 = zset->Count(&spec2);
  ASSERT_EQ(c2, 2);

  /* max exclusive */
  const ZSet::RangeByScoreSpec& spec3 = {
      .min = 2.0,
      .max = 5.0,
      .minex = false,
      .maxex = true,
  };
  const size_t c3 = zset->Count(&spec3);
  ASSERT_EQ(c3, 2);

  /* min and max exclusive */
  const ZSet::RangeByScoreSpec& spec4 = {
      .min = 1.0,
      .max = 5.0,
      .minex = true,
      .maxex = true,
  };
  const size_t c4 = zset->Count(&spec4);
  ASSERT_EQ(c4, 2);

  /* invalid spec non-exclusive, min > max */
  const ZSet::RangeByScoreSpec& spec5 = {
      .min = 6.0,
      .max = 1.0,
      .minex = false,
      .maxex = false,
  };
  const size_t c5 = zset->Count(&spec5);
  ASSERT_EQ(c5, 0);

  /* invalid spec min exclusive, min >= max */
  const ZSet::RangeByScoreSpec& spec6 = {
      .min = 1.0,
      .max = 1.0,
      .minex = true,
      .maxex = false,
  };
  const size_t c6 = zset->Count(&spec6);
  ASSERT_EQ(c6, 0);

  /* invalid spec max exclusive, min >= max */
  const ZSet::RangeByScoreSpec& spec7 = {
      .min = 1.0,
      .max = 1.0,
      .minex = false,
      .maxex = true,
  };
  const size_t c7 = zset->Count(&spec7);
  ASSERT_EQ(c7, 0);
}

TEST_F(ZSetTest, Remove) {
  bool r1 = zset->Remove("key1");
  ASSERT_TRUE(r1);
  ASSERT_EQ(zset->Size(), 3);

  int rank = zset->GetRankOfKey("key1");
  ASSERT_EQ(rank, -1);

  bool r2 = zset->Remove("key not exist");
  ASSERT_FALSE(r2);
}

const std::vector<KeyScorePair> ToKeyScorePairs(
    const std::vector<const ZSet::ZSetEntry*>& keys) {
  std::vector<std::pair<std::string, double>> pairs;
  for (int i = 0; i < keys.size(); ++i) {
    pairs.push_back({keys[i]->key, keys[i]->score});
  }
  return pairs;
}
}  // namespace zset
}  // namespace redis_simple
