#include "memory/skiplist.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace redis_simple {
namespace in_memory {
class SkiplistTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    skiplist = std::make_unique<Skiplist<std::string>>(4);
  }
  static void TearDownTestSuite() { skiplist.reset(); }

  static std::unique_ptr<Skiplist<std::string>> skiplist;
};

struct RangeByRankSpecTestCase {
  RangeByRankSpecTestCase(Skiplist<std::string>::SkiplistRangeByRankSpec spec,
                          std::vector<std::string> keys,
                          std::vector<std::string> revkeys)
      : spec(spec), keys(std::move(keys)), revkeys(std::move(revkeys)) {}
  const Skiplist<std::string>::SkiplistRangeByRankSpec spec;
  const std::vector<std::string> keys;
  const std::vector<std::string> revkeys;
};

struct RangeByKeySpecTestCase {
  RangeByKeySpecTestCase(Skiplist<std::string>::SkiplistRangeByKeySpec spec,
                         std::vector<std::string> keys,
                         std::vector<std::string> revkeys, ssize_t count)
      : spec(spec),
        keys(std::move(keys)),
        revkeys(std::move(revkeys)),
        count(count) {}
  const Skiplist<std::string>::SkiplistRangeByKeySpec spec;
  const std::vector<std::string> keys;
  const std::vector<std::string> revkeys;
  // The value used for testing Skiplist.Count(), the result is not related to
  // SkiplistLimitSpec
  const ssize_t count;
};

std::vector<RangeByRankSpecTestCase> RangeByRankSpecTestCases();
std::vector<RangeByKeySpecTestCase> RangeByKeySpecTestCases();
void ScanSkiplist(const Skiplist<std::string>* skiplist);

std::unique_ptr<Skiplist<std::string>> SkiplistTest::skiplist = nullptr;

TEST_F(SkiplistTest, Insertion) {
  ASSERT_EQ(skiplist->Insert("key1"), "key1");
  ASSERT_EQ(skiplist->Insert("key2"), "key2");
  ASSERT_EQ(skiplist->Insert("key0"), "key0");
  ASSERT_EQ(skiplist->Insert("key1"), "key1");
  ASSERT_EQ(skiplist->Size(), 3);

  ASSERT_TRUE(skiplist->Contains("key1"));
  ASSERT_TRUE(skiplist->Contains("key2"));
  ASSERT_TRUE(skiplist->Contains("key0"));
  ASSERT_TRUE(!skiplist->Contains("key_not_exist"));
}

TEST_F(SkiplistTest, Deletion) {
  ASSERT_TRUE(skiplist->Delete("key1"));
  ASSERT_FALSE(skiplist->Contains("key1"));
  ASSERT_EQ(skiplist->Size(), 2);
  ASSERT_FALSE(skiplist->Delete("key1"));
  ASSERT_EQ(skiplist->Size(), 2);

  ASSERT_TRUE(skiplist->Delete("key2"));
  ASSERT_FALSE(skiplist->Contains("key2"));
  ASSERT_EQ(skiplist->Size(), 1);
  ASSERT_FALSE(skiplist->Delete("key2"));
  ASSERT_EQ(skiplist->Size(), 1);

  ASSERT_FALSE(skiplist->Delete("key_not_exist"));
}

TEST_F(SkiplistTest, Update) {
  ASSERT_EQ("key1", skiplist->Insert("key1"));
  ASSERT_EQ("key2", skiplist->Insert("key2"));
  ASSERT_EQ("key3", skiplist->Insert("key3"));
  ASSERT_EQ(skiplist->Size(), 4);

  ASSERT_TRUE(skiplist->Update("key3", "key5"));
  ASSERT_EQ(skiplist->Size(), 4);
  ASSERT_FALSE(skiplist->Contains("key3"));
  ASSERT_TRUE(skiplist->Contains("key5"));

  ASSERT_TRUE(skiplist->Update("key1", "key4"));

  ASSERT_EQ(skiplist->Size(), 4);
  ASSERT_FALSE(skiplist->Contains("key1"));
  ASSERT_TRUE(skiplist->Contains("key4"));

  ASSERT_TRUE(!skiplist->Update("key_not_exist", "key6"));
  ASSERT_EQ(skiplist->Size(), 4);
}

TEST_F(SkiplistTest, FindKeyByRank) {
  const std::string& s0 = skiplist->FindKeyByRank(0);
  ASSERT_EQ(s0, "key0");

  const std::string& s1 = skiplist->FindKeyByRank(1);
  ASSERT_EQ(s1, "key2");

  const std::string& s2 = skiplist->FindKeyByRank(-1);
  ASSERT_EQ(s2, "key5");

  const std::string& s3 = skiplist->FindKeyByRank(-2);
  ASSERT_EQ(s3, "key4");

  const std::string& s4 = skiplist->FindKeyByRank(-4);
  ASSERT_EQ(s4, "key0");

  ASSERT_THROW(skiplist->FindKeyByRank(skiplist->Size()), std::out_of_range);
  ASSERT_THROW(skiplist->FindKeyByRank(-skiplist->Size() - 1),
               std::out_of_range);
  ASSERT_THROW(skiplist->FindKeyByRank(INT_MAX), std::out_of_range);
  ASSERT_THROW(skiplist->FindKeyByRank(INT_MIN), std::out_of_range);
}

TEST_F(SkiplistTest, FindRankOfKey) {
  ssize_t r0 = skiplist->FindRankOfKey("key0");
  ASSERT_EQ(r0, 0);

  ssize_t r1 = skiplist->FindRankOfKey("key2");
  ASSERT_EQ(r1, 1);

  ssize_t r2 = skiplist->FindRankOfKey("key5");
  ASSERT_EQ(r2, 3);

  ssize_t r3 = skiplist->FindRankOfKey("key_not_exist");
  ASSERT_EQ(r3, -1);
}

TEST_F(SkiplistTest, ArrayAccess) {
  ASSERT_EQ((*skiplist)[0], "key0");
  ASSERT_EQ((*skiplist)[1], "key2");
  ASSERT_EQ((*skiplist)[2], "key4");
  ASSERT_EQ((*skiplist)[3], "key5");
  ASSERT_EQ((*skiplist)[skiplist->Size() - 1], "key5");

  ASSERT_THROW((*skiplist)[skiplist->Size()], std::out_of_range);
  ASSERT_THROW((*skiplist)[-1], std::out_of_range);
  ASSERT_THROW((*skiplist)[INT_MAX], std::out_of_range);
  ASSERT_THROW((*skiplist)[INT_MIN], std::out_of_range);
}

TEST_F(SkiplistTest, RangeByRank) {
  const auto tests = RangeByRankSpecTestCases();
  for (const auto& test : tests) {
    ASSERT_EQ(skiplist->RangeByRank(&test.spec), test.keys);
  }
}

TEST_F(SkiplistTest, RevRangeByRank) {
  const auto tests = RangeByRankSpecTestCases();
  for (const auto& test : tests) {
    ASSERT_EQ(skiplist->RevRangeByRank(&test.spec), test.revkeys);
  }
}

TEST_F(SkiplistTest, RangeByKey) {
  const auto tests = RangeByKeySpecTestCases();
  for (const auto& test : tests) {
    ASSERT_EQ(skiplist->RangeByKey(&test.spec), test.keys);
  }
}

TEST_F(SkiplistTest, RevRangeByKey) {
  const auto tests = RangeByKeySpecTestCases();
  for (const auto& test : tests) {
    ASSERT_EQ(skiplist->RevRangeByKey(&test.spec), test.revkeys);
  }
}

TEST_F(SkiplistTest, Count) {
  const auto tests = RangeByKeySpecTestCases();
  for (const auto& test : tests) {
    ASSERT_EQ(skiplist->Count(&test.spec), test.count);
  }
}

std::vector<RangeByRankSpecTestCase> RangeByRankSpecTestCases() {
  static auto limit1 =
      std::make_unique<Skiplist<std::string>::SkiplistLimitSpec>(0, -1);
  static auto limit2 =
      std::make_unique<Skiplist<std::string>::SkiplistLimitSpec>(0, 2);
  static auto limit3 =
      std::make_unique<Skiplist<std::string>::SkiplistLimitSpec>(2, 3);
  static auto limit4 =
      std::make_unique<Skiplist<std::string>::SkiplistLimitSpec>(10, -1);

  return {
      RangeByRankSpecTestCase(Skiplist<std::string>::SkiplistRangeByRankSpec(
                                  0, 3, false, false, nullptr),
                              {"key0", "key2", "key4", "key5"},
                              {"key5", "key4", "key2", "key0"}),
      RangeByRankSpecTestCase(Skiplist<std::string>::SkiplistRangeByRankSpec(
                                  1, 3, true, false, nullptr),
                              {"key4", "key5"}, {"key2", "key0"}),
      RangeByRankSpecTestCase(Skiplist<std::string>::SkiplistRangeByRankSpec(
                                  1, 3, false, true, nullptr),
                              {"key2", "key4"}, {"key4", "key2"}),
      RangeByRankSpecTestCase(Skiplist<std::string>::SkiplistRangeByRankSpec(
                                  1, 3, false, false, limit1.get()),
                              {"key2", "key4", "key5"},
                              {"key4", "key2", "key0"}),
      RangeByRankSpecTestCase(Skiplist<std::string>::SkiplistRangeByRankSpec(
                                  1, 3, false, false, limit2.get()),
                              {"key2", "key4"}, {"key4", "key2"}),
      RangeByRankSpecTestCase(Skiplist<std::string>::SkiplistRangeByRankSpec(
                                  0, 3, false, false, limit3.get()),
                              {"key4", "key5"}, {"key2", "key0"}),
      RangeByRankSpecTestCase(Skiplist<std::string>::SkiplistRangeByRankSpec(
                                  0, 4, false, false, limit4.get()),
                              {}, {}),
      RangeByRankSpecTestCase(Skiplist<std::string>::SkiplistRangeByRankSpec(
                                  2, 1, false, false, nullptr),
                              {}, {}),
      RangeByRankSpecTestCase(Skiplist<std::string>::SkiplistRangeByRankSpec(
                                  1, 1, true, false, nullptr),
                              {}, {}),
      RangeByRankSpecTestCase(Skiplist<std::string>::SkiplistRangeByRankSpec(
                                  1, 1, false, true, nullptr),
                              {}, {}),
      RangeByRankSpecTestCase(Skiplist<std::string>::SkiplistRangeByRankSpec(
                                  100000, 1000000, false, false, nullptr),
                              {}, {})};
}

std::vector<RangeByKeySpecTestCase> RangeByKeySpecTestCases() {
  static auto limit1 =
      std::make_unique<Skiplist<std::string>::SkiplistLimitSpec>(0, -1);
  static auto limit2 =
      std::make_unique<Skiplist<std::string>::SkiplistLimitSpec>(0, 2);
  static auto limit3 =
      std::make_unique<Skiplist<std::string>::SkiplistLimitSpec>(2, 3);
  static auto limit4 =
      std::make_unique<Skiplist<std::string>::SkiplistLimitSpec>(10, -1);

  return {
      RangeByKeySpecTestCase(Skiplist<std::string>::SkiplistRangeByKeySpec(
                                 "", false, "zzzzzzz", false, nullptr),
                             {"key0", "key2", "key4", "key5"},
                             {"key5", "key4", "key2", "key0"}, 4),
      RangeByKeySpecTestCase(Skiplist<std::string>::SkiplistRangeByKeySpec(
                                 "key2", true, "key5", false, nullptr),
                             {"key4", "key5"}, {"key5", "key4"}, 2),
      RangeByKeySpecTestCase(Skiplist<std::string>::SkiplistRangeByKeySpec(
                                 "key1", true, "key5", false, nullptr),
                             {"key2", "key4", "key5"}, {"key5", "key4", "key2"},
                             3),
      RangeByKeySpecTestCase(Skiplist<std::string>::SkiplistRangeByKeySpec(
                                 "key2", false, "key5", true, nullptr),
                             {"key2", "key4"}, {"key4", "key2"}, 2),
      RangeByKeySpecTestCase(Skiplist<std::string>::SkiplistRangeByKeySpec(
                                 "key1", false, "key6", true, nullptr),
                             {"key2", "key4", "key5"}, {"key5", "key4", "key2"},
                             3),
      RangeByKeySpecTestCase(Skiplist<std::string>::SkiplistRangeByKeySpec(
                                 "key2", false, "key5", false, limit1.get()),
                             {"key2", "key4", "key5"}, {"key5", "key4", "key2"},
                             3),
      RangeByKeySpecTestCase(Skiplist<std::string>::SkiplistRangeByKeySpec(
                                 "", false, "zzzzzzz", false, limit2.get()),
                             {"key0", "key2"}, {"key5", "key4"}, 4),
      RangeByKeySpecTestCase(Skiplist<std::string>::SkiplistRangeByKeySpec(
                                 "", false, "zzzzzzz", false, limit3.get()),
                             {"key4", "key5"}, {"key2", "key0"}, 4),
      RangeByKeySpecTestCase(Skiplist<std::string>::SkiplistRangeByKeySpec(
                                 "", false, "zzzzzzz", false, limit4.get()),
                             {}, {}, 4),
      RangeByKeySpecTestCase(Skiplist<std::string>::SkiplistRangeByKeySpec(
                                 "zzzzzzz", false, "", false, nullptr),
                             {}, {}, 0),
      RangeByKeySpecTestCase(Skiplist<std::string>::SkiplistRangeByKeySpec(
                                 "key0", false, "key0", true, nullptr),
                             {}, {}, 0),
  };
}

TEST_F(SkiplistTest, Iteration) {
  auto it = Skiplist<std::string>::Iterator(skiplist.get());
  it.SeekToLast();
  ASSERT_EQ(*it, "key5");
  ASSERT_TRUE(it.Valid());
  it.Next();
  ASSERT_FALSE(it.Valid());

  it.SeekToLast();
  it.Prev();
  ASSERT_EQ(*it, "key4");

  ++it;
  ASSERT_EQ(*it, "key5");

  it.SeekToFirst();
  ASSERT_EQ(*it, "key0");
  ASSERT_TRUE(it.Valid());

  ++it;
  ASSERT_EQ(*it, "key2");
  ASSERT_TRUE(it.Valid());

  ScanSkiplist(skiplist.get());
}

void ScanSkiplist(const Skiplist<std::string>* skiplist) {
  RS_LOG_DEBUG("----start scanning skiplist----\n");
  for (auto it = skiplist->Begin(); it != skiplist->End(); ++it) {
    RS_LOG_DEBUG("%s\n", (*it).c_str());
  }
  RS_LOG_DEBUG("----end----\n");

  skiplist->Print();
}

struct Comparator {
  int operator()(const std::string& k1, const std::string& k2) const {
    return k1 < k2 ? 1 : (k1 == k2 ? 0 : -1);
  }
};

class CustomSkiplistTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    Comparator cmp;
    skiplist = std::make_unique<Skiplist<std::string, Comparator>>(4, cmp);
  }
  static void TearDownTestSuite() { skiplist.reset(); }

  static std::unique_ptr<Skiplist<std::string, Comparator>> skiplist;
};

std::unique_ptr<Skiplist<std::string, Comparator>>
    CustomSkiplistTest::skiplist = nullptr;

TEST_F(CustomSkiplistTest, Insertion) {
  ASSERT_EQ(skiplist->Insert("key1"), "key1");
  ASSERT_EQ(skiplist->Insert("key2"), "key2");
  ASSERT_EQ(skiplist->Insert("key0"), "key0");
  ASSERT_EQ(skiplist->Insert("key1"), "key1");
  ASSERT_EQ(skiplist->Size(), 3);

  ASSERT_TRUE(skiplist->Contains("key1"));
  ASSERT_TRUE(skiplist->Contains("key2"));
  ASSERT_TRUE(skiplist->Contains("key0"));
  ASSERT_TRUE(!skiplist->Contains("key_not_exist"));
}

TEST_F(CustomSkiplistTest, ArrayAccess) {
  ASSERT_EQ((*skiplist)[0], "key2");
  ASSERT_EQ((*skiplist)[1], "key1");
  ASSERT_EQ((*skiplist)[2], "key0");
  ASSERT_EQ((*skiplist)[skiplist->Size() - 1], "key0");

  ASSERT_THROW((*skiplist)[skiplist->Size()], std::out_of_range);
  ASSERT_THROW((*skiplist)[-1], std::out_of_range);
  ASSERT_THROW((*skiplist)[INT_MAX], std::out_of_range);
  ASSERT_THROW((*skiplist)[INT_MIN], std::out_of_range);
}
}  // namespace in_memory
}  // namespace redis_simple
