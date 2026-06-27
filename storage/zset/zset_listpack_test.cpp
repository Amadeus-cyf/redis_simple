#include "storage/zset/zset_listpack.h"

#include <limits>
#include <memory>

#include "gtest/gtest.h"

namespace redis_simple::zset {
class ZSetListPackTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    zset_listpack = std::make_unique<ZSetListPack>();
  }
  static void TearDownTestSuite() { zset_listpack.reset(); }

  static std::unique_ptr<ZSetListPack> zset_listpack;
};

std::unique_ptr<ZSetListPack> ZSetListPackTest::zset_listpack = nullptr;
using KeyScorePair = std::pair<std::string, double>;
std::vector<std::pair<std::string, double>> ToKeyScorePairs(
    const ZSetEntryList& keys);

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
  auto r1 = zset_listpack->GetRankOfKey("key1");
  ASSERT_EQ(r1.value_or(-1), 3);

  auto r2 = zset_listpack->GetRankOfKey("key2");
  ASSERT_EQ(r2.value_or(-1), 2);

  auto r3 = zset_listpack->GetRankOfKey("key3");
  ASSERT_EQ(r3.value_or(-1), 0);

  auto r4 = zset_listpack->GetRankOfKey("key4");
  ASSERT_EQ(r4.value_or(-1), 1);

  auto r5 = zset_listpack->GetRankOfKey("key_not_exist");
  ASSERT_FALSE(r5.has_value());
}

TEST_F(ZSetListPackTest, Update) {
  // Update score with no change in rank.
  ASSERT_FALSE(zset_listpack->InsertOrUpdate("key1", 10.0));
  ASSERT_EQ(zset_listpack->Size(), 4);
  auto r0 = zset_listpack->GetRankOfKey("key1");
  ASSERT_EQ(r0.value_or(-1), 3);

  // Update score with change in rank.
  ASSERT_FALSE(zset_listpack->InsertOrUpdate("key1", 1.0));
  ASSERT_EQ(zset_listpack->Size(), 4);
  ASSERT_FALSE(zset_listpack->InsertOrUpdate("key3", 4.0));
  ASSERT_EQ(zset_listpack->Size(), 4);

  auto r1 = zset_listpack->GetRankOfKey("key1");
  ASSERT_EQ(r1.value_or(-1), 0);

  auto r2 = zset_listpack->GetRankOfKey("key2");
  ASSERT_EQ(r2.value_or(-1), 2);

  auto r3 = zset_listpack->GetRankOfKey("key3");
  ASSERT_EQ(r3.value_or(-1), 3);

  auto r4 = zset_listpack->GetRankOfKey("key4");
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
  const RangeByRankSpec spec0(0, 3, false, false,
                              std::make_unique<LimitSpec>(0, -1), false);
  const auto p0 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec0));
  const auto e0 = std::vector<KeyScorePair>{
      {"key4", 1.0}, {"key2", 2.0}, {"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p0, e0);

  // Min exclusive
  const RangeByRankSpec spec1(1, 3, true, false,
                              std::make_unique<LimitSpec>(0, -1), false);
  const auto p1 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec1));
  const auto e1 = std::vector<KeyScorePair>{{"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p1, e1);

  // Max exclusive
  const RangeByRankSpec spec2(1, 3, false, true,
                              std::make_unique<LimitSpec>(0, -1), false);
  const auto k2 = zset_listpack->RangeByRank(&spec2);
  const auto p2 = ToKeyScorePairs(k2);
  const auto e2 = std::vector<KeyScorePair>{{"key2", 2.0}, {"key3", 4.0}};
  ASSERT_EQ(p2, e2);

  // Min and max exclusive
  const RangeByRankSpec spec3(1, 3, true, true,
                              std::make_unique<LimitSpec>(0, -1), false);
  const auto p3 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec3));
  const auto e3 = std::vector<KeyScorePair>{{"key3", 4.0}};
  ASSERT_EQ(p3, e3);

  // Limit
  const RangeByRankSpec spec4(0, 3, false, false,
                              std::make_unique<LimitSpec>(2, 3), false);
  const auto p4 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec4));
  const auto e4 = std::vector<KeyScorePair>{{"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p4, e4);

  // Reverse
  const RangeByRankSpec spec5(0, 3, false, false,
                              std::make_unique<LimitSpec>(0, -1), true);
  const auto p5 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec5));
  const auto e5 = std::vector<KeyScorePair>{
      {"key1", 5.0}, {"key3", 4.0}, {"key2", 2.0}, {"key4", 1.0}};
  ASSERT_EQ(p5, e5);

  // Reverse with limit
  const RangeByRankSpec spec6(0, 3, false, false,
                              std::make_unique<LimitSpec>(1, 2), true);
  const auto p6 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec6));
  const auto e6 = std::vector<KeyScorePair>{{"key3", 4.0}, {"key2", 2.0}};
  ASSERT_EQ(p6, e6);

  // Count = 0
  const RangeByRankSpec spec7(0, 3, false, false,
                              std::make_unique<LimitSpec>(0, 0), false);
  const auto p7 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec7));
  ASSERT_EQ(p7.size(), 0);

  // Invalid spec non-exclusive, min > max.
  const RangeByRankSpec spec8(2, 1, false, false,
                              std::make_unique<LimitSpec>(0, -1), false);
  const auto k8 = zset_listpack->RangeByRank(&spec8);
  const auto p8 = ToKeyScorePairs(k8);
  ASSERT_EQ(p8.size(), 0);

  // Invalid spec exclusive, min >= max.
  const RangeByRankSpec spec9(1, 1, false, true,
                              std::make_unique<LimitSpec>(0, -1), false);
  const auto p9 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec9));
  ASSERT_EQ(p9.size(), 0);

  // Min out of range
  const RangeByRankSpec spec10(10, 12, false, false,
                               std::make_unique<LimitSpec>(0, -1), false);
  const auto p10 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec10));
  ASSERT_EQ(p10.size(), 0);

  // Offset out of range.
  const RangeByRankSpec spec11(0, 3, false, false,
                               std::make_unique<LimitSpec>(10, -1), false);
  const auto p11 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec11));
  ASSERT_EQ(p11.size(), 0);

  // Negative index
  const RangeByRankSpec spec12(-3, -1, false, false,
                               std::make_unique<LimitSpec>(0, -1), false);
  const auto p12 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec12));
  const auto e12 =
      std::vector<KeyScorePair>{{"key2", 2.0}, {"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p12, e12);

  // Negative index, min exclusive
  const RangeByRankSpec spec13(-3, -1, true, false,
                               std::make_unique<LimitSpec>(0, -1), false);
  const auto p13 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec13));
  const auto e13 = std::vector<KeyScorePair>{{"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p13, e13);

  // Negative index, max exclusive
  const RangeByRankSpec spec14(-3, -1, false, true,
                               std::make_unique<LimitSpec>(0, -1), false);
  const auto p14 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec14));
  const auto e14 = std::vector<KeyScorePair>{{"key2", 2.0}, {"key3", 4.0}};
  ASSERT_EQ(p14, e14);

  // Negative index, reverse
  const RangeByRankSpec spec15(-3, -1, false, false,
                               std::make_unique<LimitSpec>(0, -1), true);
  const auto p15 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec15));
  const auto e15 =
      std::vector<KeyScorePair>{{"key3", 4.0}, {"key2", 2.0}, {"key4", 1.0}};
  ASSERT_EQ(p15, e15);

  // Negative index, invalid spec non-exclusive, min > max
  const RangeByRankSpec spec16(-1, -3, false, false,
                               std::make_unique<LimitSpec>(0, -1), false);
  const auto p16 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec16));
  ASSERT_EQ(p16.size(), 0);

  // Negative index, invalid spec exclusive, min >= max
  const RangeByRankSpec spec17(-1, -1, true, false,
                               std::make_unique<LimitSpec>(0, -1), false);
  const auto p17 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec17));
  ASSERT_EQ(p17.size(), 0);

  // Negative index, invalid spec, min out of range
  const RangeByRankSpec spec18(-10, 3, false, false,
                               std::make_unique<LimitSpec>(0, -1), false);
  const auto p18 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec18));
  ASSERT_EQ(p18.size(), 0);

  // Negative index, invalid spec, max out of range
  const RangeByRankSpec spec19(-1, -6, false, false,
                               std::make_unique<LimitSpec>(0, -1), false);
  const auto p19 = ToKeyScorePairs(zset_listpack->RangeByRank(&spec19));
  ASSERT_EQ(p19.size(), 0);
}

TEST_F(ZSetListPackTest, RangeByScore) {
  // Base
  const RangeByScoreSpec spec0(1.0, 5.0, false, false,
                               std::make_unique<LimitSpec>(0, -1), false);
  const auto p0 = ToKeyScorePairs(zset_listpack->RangeByScore(&spec0));
  const auto e0 = std::vector<KeyScorePair>{
      {"key4", 1.0}, {"key2", 2.0}, {"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p0, e0);

  // Infinity
  const RangeByScoreSpec spec1(-std::numeric_limits<double>::infinity(),
                               std::numeric_limits<double>::infinity(), false,
                               false, std::make_unique<LimitSpec>(0, -1),
                               false);
  const auto p1 = ToKeyScorePairs(zset_listpack->RangeByScore(&spec1));
  const auto e1 = std::vector<KeyScorePair>{
      {"key4", 1.0}, {"key2", 2.0}, {"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p1, e1);

  // Min exclusive
  const RangeByScoreSpec spec2(2.0, 5.0, true, false,
                               std::make_unique<LimitSpec>(0, -1), false);
  const auto p2 = ToKeyScorePairs(zset_listpack->RangeByScore(&spec2));
  const auto e2 = std::vector<KeyScorePair>{{"key3", 4.0}, {"key1", 5.0}};
  ASSERT_EQ(p2, e2);

  // Max exclusive
  const RangeByScoreSpec spec3(2.0, 5.0, false, true,
                               std::make_unique<LimitSpec>(0, -1), false);
  const auto p3 = ToKeyScorePairs(zset_listpack->RangeByScore(&spec3));
  const auto e3 = std::vector<KeyScorePair>{{"key2", 2.0}, {"key3", 4.0}};
  ASSERT_EQ(p3, e3);

  // Min and max exclusive
  const RangeByScoreSpec spec4(2.0, 5.0, true, true,
                               std::make_unique<LimitSpec>(0, -1), false);
  const auto p4 = ToKeyScorePairs(zset_listpack->RangeByScore(&spec4));
  const auto e4 = std::vector<KeyScorePair>{{"key3", 4.0}};
  ASSERT_EQ(p4, e4);

  // With limit
  const RangeByScoreSpec spec5(1.0, 6.0, false, false,
                               std::make_unique<LimitSpec>(1, 2), false);
  const auto p5 = ToKeyScorePairs(zset_listpack->RangeByScore(&spec5));
  const auto e5 = std::vector<KeyScorePair>{{"key2", 2.0}, {"key3", 4.0}};
  ASSERT_EQ(p5, e5);

  // Reverse.
  const RangeByScoreSpec spec6(2.0, 6.0, false, false,
                               std::make_unique<LimitSpec>(0, -1), true);
  const auto p6 = ToKeyScorePairs(zset_listpack->RangeByScore(&spec6));
  const auto e6 =
      std::vector<KeyScorePair>{{"key1", 5.0}, {"key3", 4.0}, {"key2", 2.0}};
  ASSERT_EQ(p6, e6);

  // Reverse with limit
  const RangeByScoreSpec spec7(1.0, 6.0, false, false,
                               std::make_unique<LimitSpec>(1, 2), true);
  const auto p7 = ToKeyScorePairs(zset_listpack->RangeByScore(&spec7));
  const auto e7 = std::vector<KeyScorePair>{{"key3", 4.0}, {"key2", 2.0}};
  ASSERT_EQ(p7, e7);

  // Count = 0
  const RangeByScoreSpec spec8(1.0, 6.0, false, false,
                               std::make_unique<LimitSpec>(0, 0), false);
  const auto p8 = ToKeyScorePairs(zset_listpack->RangeByScore(&spec8));
  ASSERT_EQ(p8.size(), 0);

  // Invalid spec, non-exclusive, min >= max.
  const RangeByScoreSpec spec9(6.0, 1.0, false, false,
                               std::make_unique<LimitSpec>(0, -1), false);
  const auto p9 = ToKeyScorePairs(zset_listpack->RangeByScore(&spec9));
  ASSERT_EQ(p9.size(), 0);

  // Invalid spec, min exclusive, min >= max.
  const RangeByScoreSpec spec10(1.0, 1.0, true, false,
                                std::make_unique<LimitSpec>(0, -1), false);
  const auto p10 = ToKeyScorePairs(zset_listpack->RangeByScore(&spec10));
  ASSERT_EQ(p10.size(), 0);

  // Invalid spec, max exclusive, min >= max.
  const RangeByScoreSpec spec11(1.0, 1.0, false, true,
                                std::make_unique<LimitSpec>(0, -1), false);
  const auto p11 = ToKeyScorePairs(zset_listpack->RangeByScore(&spec11));
  ASSERT_EQ(p11.size(), 0);

  // Invalid spec, offset out of range.
  const RangeByScoreSpec spec12(1.0, 6.0, false, false,
                                std::make_unique<LimitSpec>(5, 0), false);
  const auto p12 = ToKeyScorePairs(zset_listpack->RangeByScore(&spec12));
  ASSERT_EQ(p12.size(), 0);
}

TEST_F(ZSetListPackTest, Count) {
  // Base
  const RangeByScoreSpec spec1(1.0, 6.0, false, false);
  const size_t c1 = zset_listpack->Count(&spec1);
  ASSERT_EQ(c1, 4);

  // Min exclusive
  const RangeByScoreSpec spec2(2.0, 5.0, true, false);
  const size_t c2 = zset_listpack->Count(&spec2);
  ASSERT_EQ(c2, 2);

  // Max exclusive
  const RangeByScoreSpec spec3(2.0, 5.0, false, true);
  const size_t c3 = zset_listpack->Count(&spec3);
  ASSERT_EQ(c3, 2);

  // Min and max exclusive
  const RangeByScoreSpec spec4(1.0, 5.0, true, true);
  const size_t c4 = zset_listpack->Count(&spec4);
  ASSERT_EQ(c4, 2);

  // Invalid spec non-exclusive, min > max.
  const RangeByScoreSpec spec5(6.0, 1.0, false, false);
  const size_t c5 = zset_listpack->Count(&spec5);
  ASSERT_EQ(c5, 0);

  // Invalid spec min exclusive, min >= max.
  const RangeByScoreSpec spec6(1.0, 1.0, true, false);
  const size_t c6 = zset_listpack->Count(&spec6);
  ASSERT_EQ(c6, 0);

  // Invalid spec max exclusive, min >= max.
  const RangeByScoreSpec spec7(1.0, 1.0, false, true);
  const size_t c7 = zset_listpack->Count(&spec7);
  ASSERT_EQ(c7, 0);
}

TEST_F(ZSetListPackTest, Delete) {
  bool r1 = zset_listpack->Delete("key1");
  ASSERT_TRUE(r1);
  ASSERT_EQ(zset_listpack->Size(), 3);

  auto rank = zset_listpack->GetRankOfKey("key1");
  ASSERT_FALSE(rank.has_value());

  bool r2 = zset_listpack->Delete("key not exist");
  ASSERT_FALSE(r2);
}
}  // namespace redis_simple::zset
