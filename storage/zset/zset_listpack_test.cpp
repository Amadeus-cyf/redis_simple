#include "storage/zset/zset_listpack.h"

#include <limits>

#include "gtest/gtest.h"

namespace redis_simple {
namespace zset {
class ZSetListPackTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { zset_listpack = new ZSetListPack(); }
  static void TearDownTestSuit() {
    delete zset_listpack;
    zset_listpack = nullptr;
  }
  static ZSetListPack* zset_listpack;
};

ZSetListPack* ZSetListPackTest::zset_listpack;
using KeyScorePair = std::pair<std::string, double>;
const std::vector<std::pair<std::string, double>> ToKeyScorePairs(
    const std::vector<const ZSetEntry*>& keys);

TEST_F(ZSetListPackTest, Add) {
  ASSERT_TRUE(zset_listpack->InsertOrUpdate("key1", 3.0));
  ASSERT_EQ(zset_listpack->Size(), 1);

  ASSERT_TRUE(zset_listpack->InsertOrUpdate("key2", 2.0));
  ASSERT_EQ(zset_listpack->Size(), 2);

  ASSERT_TRUE(zset_listpack->InsertOrUpdate("key3", 1.0));
  ASSERT_EQ(zset_listpack->Size(), 3);

  ASSERT_TRUE(zset_listpack->InsertOrUpdate("key4", 1.0));
  ASSERT_EQ(zset_listpack->Size(), 4);
}

TEST_F(ZSetListPackTest, GetRankOfKey) {
  std::optional<size_t> r1 = zset_listpack->GetRankOfKey("key1");
  ASSERT_EQ(r1.value_or(-1), 3);

  std::optional<size_t> r2 = zset_listpack->GetRankOfKey("key2");
  ASSERT_EQ(r2.value_or(-1), 2);

  std::optional<size_t> r3 = zset_listpack->GetRankOfKey("key3");
  ASSERT_EQ(r3.value_or(-1), 0);

  std::optional<size_t> r4 = zset_listpack->GetRankOfKey("key4");
  ASSERT_EQ(r4.value_or(-1), 1);

  std::optional<size_t> r5 = zset_listpack->GetRankOfKey("key_not_exist");
  ASSERT_FALSE(r5.has_value());
}

TEST_F(ZSetListPackTest, Update) {
  // Update score with no change in rank.
  ASSERT_FALSE(zset_listpack->InsertOrUpdate("key1", 10.0));
  ASSERT_EQ(zset_listpack->Size(), 4);
  std::optional<size_t> r0 = zset_listpack->GetRankOfKey("key1");
  ASSERT_EQ(r0.value_or(-1), 3);

  // Update score with change in rank.
  ASSERT_FALSE(zset_listpack->InsertOrUpdate("key1", 1.0));
  ASSERT_EQ(zset_listpack->Size(), 4);
  ASSERT_FALSE(zset_listpack->InsertOrUpdate("key3", 4.0));
  ASSERT_EQ(zset_listpack->Size(), 4);

  std::optional<size_t> r1 = zset_listpack->GetRankOfKey("key1");
  ASSERT_EQ(r1.value_or(-1), 0);

  std::optional<size_t> r2 = zset_listpack->GetRankOfKey("key2");
  ASSERT_EQ(r2.value_or(-1), 2);

  std::optional<size_t> r3 = zset_listpack->GetRankOfKey("key3");
  ASSERT_EQ(r3.value_or(-1), 3);

  std::optional<size_t> r4 = zset_listpack->GetRankOfKey("key4");
  ASSERT_EQ(r4.value_or(-1), 1);

  ASSERT_FALSE(zset_listpack->InsertOrUpdate("key1", 5.0));
  ASSERT_EQ(zset_listpack->Size(), 4);

  r1 = zset_listpack->GetRankOfKey("key1");
  ASSERT_EQ(r1, 3);

  r4 = zset_listpack->GetRankOfKey("key4");
  ASSERT_EQ(r4, 0);
}

TEST_F(ZSetListPackTest, RangeByRank) {
  // Base
  const RangeByRankSpec& spec0 = {
      .min = 0,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p0 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec0));
  const std::vector<KeyScorePair>& e0 = {
      {"key4", 1.0}, {"key2", 2.0}, {"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p0, e0);

  // Min exclusive
  const RangeByRankSpec& spec1 = {
      .min = 1,
      .max = 3,
      .minex = true,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p1 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec1));
  const std::vector<KeyScorePair>& e1 = {{"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p1, e1);

  // Max exclusive
  const RangeByRankSpec& spec2 = {
      .min = 1,
      .max = 3,
      .minex = false,
      .maxex = true,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<const ZSetEntry*>& k2 = zset_listpack->RangeByRank(&spec2);
  const std::vector<KeyScorePair>& p2 = ToKeyScorePairs(k2);
  const std::vector<KeyScorePair>& e2 = {{"key2", 2.0}, {"key3", 4.0}};
  ASSERT_EQ(p2, e2);

  // Min and max exclusive
  const RangeByRankSpec& spec3 = {
      .min = 1,
      .max = 3,
      .minex = true,
      .maxex = true,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p3 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec3));
  const std::vector<KeyScorePair>& e3 = {{"key3", 4.0}};
  ASSERT_EQ(p3, e3);

  // Limit
  const RangeByRankSpec& spec4 = {
      .min = 0,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(2, 3),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p4 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec4));
  const std::vector<KeyScorePair>& e4 = {{"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p4, e4);

  // Reverse
  const RangeByRankSpec& spec5 = {
      .min = 0,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = true,
  };
  const std::vector<KeyScorePair>& p5 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec5));
  const std::vector<KeyScorePair>& e5 = {
      {"key1", 5.0}, {"key3", 4.0}, {"key2", 2.0}, {"key4", 1.0}};
  ASSERT_EQ(p5, e5);

  // Reverse with limit
  const RangeByRankSpec& spec6 = {
      .min = 0,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(1, 2),
      .reverse = true,
  };
  const std::vector<KeyScorePair>& p6 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec6));
  const std::vector<KeyScorePair>& e6 = {{"key3", 4.0}, {"key2", 2.0}};
  ASSERT_EQ(p6, e6);

  // Count = 0
  const RangeByRankSpec& spec7 = {
      .min = 0,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, 0),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p7 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec7));
  ASSERT_EQ(p7.size(), 0);

  // Invalid spec non-exclusive, min > max.
  const RangeByRankSpec& spec8 = {
      .min = 2,
      .max = 1,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<const ZSetEntry*>& k8 = zset_listpack->RangeByRank(&spec8);
  const std::vector<KeyScorePair>& p8 = ToKeyScorePairs(k8);
  ASSERT_EQ(p8.size(), 0);

  // Invalid spec exclusive, min >= max.
  const RangeByRankSpec& spec9 = {
      .min = 1,
      .max = 1,
      .minex = false,
      .maxex = true,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p9 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec9));
  ASSERT_EQ(p9.size(), 0);

  // Min out of range
  const RangeByRankSpec& spec10 = {
      .min = 10,
      .max = 12,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p10 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec10));
  ASSERT_EQ(p10.size(), 0);

  // Offset out of range.
  const RangeByRankSpec& spec11 = {
      .min = 0,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(10, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p11 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec11));
  ASSERT_EQ(p11.size(), 0);

  // Negative index
  const RangeByRankSpec& spec12 = {
      .min = -3,
      .max = -1,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p12 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec12));
  const std::vector<KeyScorePair>& e12 = {
      {"key2", 2.0}, {"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p12, e12);

  // Negative index, min exclusive
  const RangeByRankSpec& spec13 = {
      .min = -3,
      .max = -1,
      .minex = true,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p13 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec13));
  const std::vector<KeyScorePair>& e13 = {{"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p13, e13);

  // Negative index, max exclusive
  const RangeByRankSpec& spec14 = {
      .min = -3,
      .max = -1,
      .minex = false,
      .maxex = true,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p14 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec14));
  const std::vector<KeyScorePair>& e14 = {{"key2", 2.0}, {"key3", 4.0}};
  ASSERT_EQ(p14, e14);

  // Negative index, reverse
  const RangeByRankSpec& spec15 = {
      .min = -3,
      .max = -1,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = true,
  };
  const std::vector<KeyScorePair>& p15 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec15));
  const std::vector<KeyScorePair>& e15 = {
      {"key3", 4.0}, {"key2", 2.0}, {"key4", 1.0}};
  ASSERT_EQ(p15, e15);

  // Negative index, invalid spec non-exclusive, min > max
  const RangeByRankSpec& spec16 = {
      .min = -1,
      .max = -3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p16 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec16));
  ASSERT_EQ(p16.size(), 0);

  // Negative index, invalid spec exclusive, min >= max
  const RangeByRankSpec& spec17 = {
      .min = -1,
      .max = -1,
      .minex = true,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p17 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec17));
  ASSERT_EQ(p17.size(), 0);

  // Negative index, invalid spec, min out of range
  const RangeByRankSpec& spec18 = {
      .min = -10,
      .max = 3,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p18 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec18));
  ASSERT_EQ(p18.size(), 0);

  // Negative index, invalid spec, max out of range
  const RangeByRankSpec& spec19 = {
      .min = -1,
      .max = -6,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p19 =
      ToKeyScorePairs(zset_listpack->RangeByRank(&spec19));
  ASSERT_EQ(p19.size(), 0);
}

TEST_F(ZSetListPackTest, RangeByScore) {
  // Base
  const RangeByScoreSpec& spec0 = {
      .min = 1.0,
      .max = 5.0,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p0 =
      ToKeyScorePairs(zset_listpack->RangeByScore(&spec0));
  const std::vector<KeyScorePair>& e0 = {
      {"key4", 1.0}, {"key2", 2.0}, {"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p0, e0);

  // Infinity
  const RangeByScoreSpec& spec1 = {
      .min = -std::numeric_limits<double>::infinity(),
      .max = std::numeric_limits<double>::infinity(),
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p1 =
      ToKeyScorePairs(zset_listpack->RangeByScore(&spec1));
  const std::vector<KeyScorePair>& e1 = {
      {"key4", 1.0}, {"key2", 2.0}, {"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p1, e1);

  // Min exclusive
  const RangeByScoreSpec& spec2 = {
      .min = 2.0,
      .max = 5.0,
      .minex = true,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p2 =
      ToKeyScorePairs(zset_listpack->RangeByScore(&spec2));
  const std::vector<KeyScorePair>& e2 = {{"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p2, e2);

  // Max exclusive
  const RangeByScoreSpec& spec3 = {
      .min = 2.0,
      .max = 5.0,
      .minex = false,
      .maxex = true,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p3 =
      ToKeyScorePairs(zset_listpack->RangeByScore(&spec3));
  const std::vector<KeyScorePair>& e3 = {{"key2", 2.0}, {"key3", 4.0}};
  ASSERT_EQ(p3, e3);

  // Min and max exclusive
  const RangeByScoreSpec& spec4 = {
      .min = 2.0,
      .max = 5.0,
      .minex = true,
      .maxex = true,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p4 =
      ToKeyScorePairs(zset_listpack->RangeByScore(&spec4));
  const std::vector<KeyScorePair>& e4 = {{"key3", 4.0}};
  ASSERT_EQ(p4, e4);

  // With limit
  const RangeByScoreSpec& spec5 = {
      .min = 1.0,
      .max = 6.0,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(1, 2),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p5 =
      ToKeyScorePairs(zset_listpack->RangeByScore(&spec5));
  const std::vector<KeyScorePair>& e5 = {{"key2", 2.0}, {"key3", 4.0}};
  ASSERT_EQ(p5, e5);

  // Reverse.
  const RangeByScoreSpec& spec6 = {
      .min = 2.0,
      .max = 6.0,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = true,
  };
  const std::vector<KeyScorePair>& p6 =
      ToKeyScorePairs(zset_listpack->RangeByScore(&spec6));
  const std::vector<KeyScorePair>& e6 = {
      {"key1", 5.0}, {"key3", 4.0}, {"key2", 2.0}};
  ASSERT_EQ(p6, e6);

  // Reverse with limit
  const RangeByScoreSpec& spec7 = {
      .min = 1.0,
      .max = 6.0,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(1, 2),
      .reverse = true,
  };
  const std::vector<KeyScorePair>& p7 =
      ToKeyScorePairs(zset_listpack->RangeByScore(&spec7));
  const std::vector<KeyScorePair>& e7 = {{"key3", 4.0}, {"key2", 2.0}};
  ASSERT_EQ(p7, e7);

  // Count = 0
  const RangeByScoreSpec& spec8 = {
      .min = 1.0,
      .max = 6.0,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, 0),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p8 =
      ToKeyScorePairs(zset_listpack->RangeByScore(&spec8));
  ASSERT_EQ(p8.size(), 0);

  // Invalid spec, non-exclusive, min >= max.
  const RangeByScoreSpec& spec9 = {
      .min = 6.0,
      .max = 1.0,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p9 =
      ToKeyScorePairs(zset_listpack->RangeByScore(&spec9));
  ASSERT_EQ(p9.size(), 0);

  // Invalid spec, min exclusive, min >= max.
  const RangeByScoreSpec& spec10 = {
      .min = 1.0,
      .max = 1.0,
      .minex = true,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p10 =
      ToKeyScorePairs(zset_listpack->RangeByScore(&spec10));
  ASSERT_EQ(p10.size(), 0);

  // Invalid spec, max exclusive, min >= max.
  const RangeByScoreSpec& spec11 = {
      .min = 1.0,
      .max = 1.0,
      .minex = false,
      .maxex = true,
      .limit = std::make_unique<LimitSpec>(0, -1),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p11 =
      ToKeyScorePairs(zset_listpack->RangeByScore(&spec11));
  ASSERT_EQ(p11.size(), 0);

  // Invalid spec, offset out of range.
  const RangeByScoreSpec& spec12 = {
      .min = 1.0,
      .max = 6.0,
      .minex = false,
      .maxex = false,
      .limit = std::make_unique<LimitSpec>(5, 0),
      .reverse = false,
  };
  const std::vector<KeyScorePair>& p12 =
      ToKeyScorePairs(zset_listpack->RangeByScore(&spec12));
  ASSERT_EQ(p12.size(), 0);
}

TEST_F(ZSetListPackTest, Count) {
  // Base
  const RangeByScoreSpec& spec1 = {
      .min = 1.0,
      .max = 6.0,
      .minex = false,
      .maxex = false,
  };
  const size_t c1 = zset_listpack->Count(&spec1);
  ASSERT_EQ(c1, 4);

  // Min exclusive
  const RangeByScoreSpec& spec2 = {
      .min = 2.0,
      .max = 5.0,
      .minex = true,
      .maxex = false,
  };
  const size_t c2 = zset_listpack->Count(&spec2);
  ASSERT_EQ(c2, 2);

  // Max exclusive
  const RangeByScoreSpec& spec3 = {
      .min = 2.0,
      .max = 5.0,
      .minex = false,
      .maxex = true,
  };
  const size_t c3 = zset_listpack->Count(&spec3);
  ASSERT_EQ(c3, 2);

  // Min and max exclusive
  const RangeByScoreSpec& spec4 = {
      .min = 1.0,
      .max = 5.0,
      .minex = true,
      .maxex = true,
  };
  const size_t c4 = zset_listpack->Count(&spec4);
  ASSERT_EQ(c4, 2);

  // Invalid spec non-exclusive, min > max.
  const RangeByScoreSpec& spec5 = {
      .min = 6.0,
      .max = 1.0,
      .minex = false,
      .maxex = false,
  };
  const size_t c5 = zset_listpack->Count(&spec5);
  ASSERT_EQ(c5, 0);

  // Invalid spec min exclusive, min >= max.
  const RangeByScoreSpec& spec6 = {
      .min = 1.0,
      .max = 1.0,
      .minex = true,
      .maxex = false,
  };
  const size_t c6 = zset_listpack->Count(&spec6);
  ASSERT_EQ(c6, 0);

  // Invalid spec max exclusive, min >= max.
  const RangeByScoreSpec& spec7 = {
      .min = 1.0,
      .max = 1.0,
      .minex = false,
      .maxex = true,
  };
  const size_t c7 = zset_listpack->Count(&spec7);
  ASSERT_EQ(c7, 0);
}

TEST_F(ZSetListPackTest, Delete) {
  bool r1 = zset_listpack->Delete("key1");
  ASSERT_TRUE(r1);
  ASSERT_EQ(zset_listpack->Size(), 3);

  std::optional<size_t> rank = zset_listpack->GetRankOfKey("key1");
  ASSERT_FALSE(rank.has_value());

  bool r2 = zset_listpack->Delete("key not exist");
  ASSERT_FALSE(r2);
}
}  // namespace zset
}  // namespace redis_simple
