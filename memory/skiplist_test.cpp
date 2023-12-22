#include "memory/skiplist.h"

#include <gtest/gtest.h>

#include <string>

namespace redis_simple {
namespace in_memory {
class SkiplistTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { skiplist = new Skiplist<std::string>(4); }
  static void TearDownTestSuite() {
    delete skiplist;
    skiplist = nullptr;
  }
  static Skiplist<std::string>* skiplist;
};

struct RangeByRankSpecTestCase {
  const Skiplist<std::string>::SkiplistRangeByRankSpec spec;
  const std::vector<std::string> keys;
  const std::vector<std::string> revkeys;
  const int count;
};

struct RangeByKeySpecTestCase {
  const Skiplist<std::string>::SkiplistRangeByKeySpec spec;
  const std::vector<std::string> keys;
  const std::vector<std::string> revkeys;
  const long count;
};

const std::vector<const RangeByRankSpecTestCase> RangeByRankSpecTestCases();
const std::vector<const RangeByKeySpecTestCase> RangeByKeySpecTestCases();
void ScanSkiplist(const Skiplist<std::string>* skiplist);

Skiplist<std::string>* SkiplistTest::skiplist;

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

TEST_F(SkiplistTest, GetKeyByRank) {
  const std::string& s0 = skiplist->GetKeyByRank(0);
  ASSERT_EQ(s0, "key0");

  const std::string& s1 = skiplist->GetKeyByRank(1);
  ASSERT_EQ(s1, "key2");

  const std::string& s2 = skiplist->GetKeyByRank(-1);
  ASSERT_EQ(s2, "key5");

  const std::string& s3 = skiplist->GetKeyByRank(-2);
  ASSERT_EQ(s3, "key4");

  const std::string& s4 = skiplist->GetKeyByRank(-4);
  ASSERT_EQ(s4, "key0");

  ASSERT_THROW(skiplist->GetKeyByRank(skiplist->Size()), std::out_of_range);
  ASSERT_THROW(skiplist->GetKeyByRank(-skiplist->Size() - 1),
               std::out_of_range);
  ASSERT_THROW(skiplist->GetKeyByRank(INT_MAX), std::out_of_range);
  ASSERT_THROW(skiplist->GetKeyByRank(INT_MIN), std::out_of_range);
}

TEST_F(SkiplistTest, GetRankofKey) {
  ssize_t r0 = skiplist->GetRankofKey("key0");
  ASSERT_EQ(r0, 0);

  ssize_t r1 = skiplist->GetRankofKey("key2");
  ASSERT_EQ(r1, 1);

  ssize_t r2 = skiplist->GetRankofKey("key5");
  ASSERT_EQ(r2, 3);

  ssize_t r3 = skiplist->GetRankofKey("key_not_exist");
  ASSERT_EQ(r3, -1);
}

TEST_F(SkiplistTest, GetKeysGt) {
  const std::vector<std::string>& k0 = skiplist->GetKeysGt("key0");
  ASSERT_EQ(k0, std::vector<std::string>({"key2", "key4", "key5"}));

  const std::vector<std::string>& k1 = skiplist->GetKeysGt("key3");
  ASSERT_EQ(k1, std::vector<std::string>({"key4", "key5"}));

  const std::vector<std::string>& k2 = skiplist->GetKeysGt("key6");
  ASSERT_EQ(k2.size(), 0);

  const std::vector<std::string>& k3 = skiplist->GetKeysGt("abc");
  ASSERT_EQ(k3, std::vector<std::string>({"key0", "key2", "key4", "key5"}));
}

TEST_F(SkiplistTest, GetKeysGte) {
  const std::vector<std::string>& k0 = skiplist->GetKeysGte("key0");
  ASSERT_EQ(k0, std::vector<std::string>({"key0", "key2", "key4", "key5"}));

  const std::vector<std::string>& k1 = skiplist->GetKeysGte("key3");
  ASSERT_EQ(k1, std::vector<std::string>({"key4", "key5"}));

  const std::vector<std::string>& k2 = skiplist->GetKeysGte("key6");
  ASSERT_EQ(k2.size(), 0);

  const std::vector<std::string>& k3 = skiplist->GetKeysGte("key5");
  ASSERT_EQ(k3, std::vector<std::string>({"key5"}));
}

TEST_F(SkiplistTest, GetKeysLt) {
  const std::vector<std::string>& k0 = skiplist->GetKeysLt("key5");
  ASSERT_EQ(k0, std::vector<std::string>({"key0", "key2", "key4"}));

  const std::vector<std::string>& k1 = skiplist->GetKeysLt("key3");
  ASSERT_EQ(k1, std::vector<std::string>({"key0", "key2"}));

  const std::vector<std::string>& k2 = skiplist->GetKeysLt("key2");
  ASSERT_EQ(k2, std::vector<std::string>({"key0"}));

  const std::vector<std::string>& k3 = skiplist->GetKeysLt("key0");
  ASSERT_EQ(k3.size(), 0);

  const std::vector<std::string>& k4 = skiplist->GetKeysLt("key6");
  ASSERT_EQ(k4, std::vector<std::string>({"key0", "key2", "key4", "key5"}));
}

TEST_F(SkiplistTest, GetKeysLte) {
  const std::vector<std::string>& k0 = skiplist->GetKeysLte("key5");
  ASSERT_EQ(k0, std::vector<std::string>({"key0", "key2", "key4", "key5"}));

  const std::vector<std::string>& k1 = skiplist->GetKeysLte("key3");
  ASSERT_EQ(k1, std::vector<std::string>({"key0", "key2"}));

  const std::vector<std::string>& k2 = skiplist->GetKeysLte("key2");
  ASSERT_EQ(k2, std::vector<std::string>({"key0", "key2"}));

  const std::vector<std::string>& k3 = skiplist->GetKeysLte("key0");
  ASSERT_EQ(k3, std::vector<std::string>({"key0"}));

  const std::vector<std::string>& k4 = skiplist->GetKeysLte("abc");
  ASSERT_EQ(k4.size(), 0);
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
  const std::vector<const RangeByRankSpecTestCase>& tests =
      RangeByRankSpecTestCases();
  for (const RangeByRankSpecTestCase& test : tests) {
    ASSERT_EQ(skiplist->RangeByRank(&test.spec), test.keys);
  }
}

TEST_F(SkiplistTest, RevRangeByRank) {
  const std::vector<const RangeByRankSpecTestCase>& tests =
      RangeByRankSpecTestCases();
  for (const RangeByRankSpecTestCase& test : tests) {
    ASSERT_EQ(skiplist->RevRangeByRank(&test.spec), test.revkeys);
  }
}

TEST_F(SkiplistTest, RangeByKey) {
  const std::vector<const RangeByKeySpecTestCase>& tests =
      RangeByKeySpecTestCases();
  for (const RangeByKeySpecTestCase& test : tests) {
    const std::vector<const RangeByKeySpecTestCase>& tests =
        RangeByKeySpecTestCases();
    for (const RangeByKeySpecTestCase& test : tests) {
      ASSERT_EQ(skiplist->RangeByKey(&test.spec), test.keys);
    }
  }
}

TEST_F(SkiplistTest, RevRangeByKey) {
  const std::vector<const RangeByKeySpecTestCase>& tests =
      RangeByKeySpecTestCases();
  for (const RangeByKeySpecTestCase& test : tests) {
    const std::vector<const RangeByKeySpecTestCase>& tests =
        RangeByKeySpecTestCases();
    for (const RangeByKeySpecTestCase& test : tests) {
      ASSERT_EQ(skiplist->RevRangeByKey(&test.spec), test.revkeys);
    }
  }
}

TEST_F(SkiplistTest, Count) {
  const std::vector<const RangeByRankSpecTestCase>& tests =
      RangeByRankSpecTestCases();
  for (const RangeByRankSpecTestCase& test : tests) {
    ASSERT_EQ(skiplist->Count(&test.spec), test.count);
  }
}

const std::vector<const RangeByRankSpecTestCase> RangeByRankSpecTestCases() {
  static auto limit1 =
      std::unique_ptr<Skiplist<std::string>::SkiplistLimitSpec>(
          new Skiplist<std::string>::SkiplistLimitSpec({
              .offset = 0,
              .count = -1,
          }));
  static auto limit2 =
      std::unique_ptr<Skiplist<std::string>::SkiplistLimitSpec>(
          new Skiplist<std::string>::SkiplistLimitSpec({
              .offset = 0,
              .count = 2,
          }));
  static auto limit3 =
      std::unique_ptr<Skiplist<std::string>::SkiplistLimitSpec>(
          new Skiplist<std::string>::SkiplistLimitSpec({
              .offset = 2,
              .count = 3,
          }));
  static auto limit4 =
      std::unique_ptr<Skiplist<std::string>::SkiplistLimitSpec>(
          new Skiplist<std::string>::SkiplistLimitSpec({
              .offset = 10,
              .count = -1,
          }));

  return {{
              /* base */
              .spec = {.min = 0,
                       .max = 3,
                       .minex = false,
                       .maxex = false,
                       .limit = nullptr},
              .keys = {"key0", "key2", "key4", "key5"},
              .revkeys = {"key5", "key4", "key2", "key0"},
              .count = 4,
          },
          {
              /* min exclusive */
              .spec = {.min = 1,
                       .max = 3,
                       .minex = true,
                       .maxex = false,
                       .limit = nullptr},
              .keys = {"key4", "key5"},
              .revkeys = {"key2", "key0"},
              .count = 2,
          },
          {
              /* max exclusive */
              .spec = {.min = 1,
                       .max = 3,
                       .minex = false,
                       .maxex = true,
                       .limit = nullptr},
              .keys = {"key2", "key4"},
              .revkeys = {"key4", "key2"},
              .count = 2,
          },
          {
              /* count = -1, return all keys in the range */
              .spec =
                  {
                      .min = 1,
                      .max = 3,
                      .minex = false,
                      .maxex = false,
                      .limit = limit1.get(),
                  },
              .keys = {"key2", "key4", "key5"},
              .revkeys = {"key4", "key2", "key0"},
              .count = 3,
          },
          {
              /* negative */
              .spec =
                  {
                      .min = -3,
                      .max = -1,
                      .minex = false,
                      .maxex = false,
                      .limit = limit1.get(),
                  },
              .keys = {"key2", "key4", "key5"},
              .revkeys = {"key4", "key2", "key0"},
              .count = 3,
          },
          {
              /* with count */
              .spec =
                  {
                      .min = 1,
                      .max = 3,
                      .minex = false,
                      .maxex = false,
                      .limit = limit2.get(),
                  },
              .keys = {"key2", "key4"},
              .revkeys = {"key4", "key2"},
              .count = 2,
          },
          {
              /* with offset */
              .spec =
                  {
                      .min = 0,
                      .max = 3,
                      .minex = false,
                      .maxex = false,
                      .limit = limit3.get(),
                  },
              .keys = {"key4", "key5"},
              .revkeys = {"key2", "key0"},
              .count = 2,
          },
          {
              /* offset out of range */
              .spec =
                  {
                      .min = 0,
                      .max = 4,
                      .minex = false,
                      .maxex = false,
                      .limit = limit4.get(),
                  },
              .keys = {},
              .revkeys = {},
              .count = 0,
          },
          {
              /* invalid spec, non-exclusive, min > max */
              .spec =
                  {
                      .min = 2,
                      .max = 1,
                      .minex = false,
                      .maxex = false,
                      .limit = nullptr,
                  },
              .keys = {},
              .revkeys = {},
              .count = -1,
          },
          {
              /* invalid spec, min exclusive, min == max */
              .spec =
                  {
                      .min = 1,
                      .max = 1,
                      .minex = true,
                      .maxex = false,
                      .limit = nullptr,
                  },
              .keys = {},
              .revkeys = {},
              .count = -1,
          },
          {
              /* invalid spec, max exclusive, min == max */
              .spec =
                  {
                      .min = 1,
                      .max = 1,
                      .minex = false,
                      .maxex = true,
                      .limit = nullptr,
                  },
              .keys = {},
              .revkeys = {},
              .count = -1,
          },
          {
              /* invalid spec, negative index, non-exclusive, min > max */
              .spec =
                  {
                      .min = -1,
                      .max = -3,
                      .minex = false,
                      .maxex = false,
                      .limit = nullptr,
                  },
              .keys = {},
              .revkeys = {},
              .count = -1,
          },
          {
              /* invalid spec, negative index, min-exclusive, min == max */
              .spec =
                  {
                      .min = -1,
                      .max = -1,
                      .minex = true,
                      .maxex = false,
                      .limit = nullptr,
                  },
              .keys = {},
              .revkeys = {},
              .count = -1,
          },
          {
              /* invalid spec, negative index, min-exclusive, min == max */
              .spec =
                  {
                      .min = -1,
                      .max = -1,
                      .minex = false,
                      .maxex = true,
                      .limit = nullptr,
                  },
              .keys = {},
              .revkeys = {},
              .count = -1,
          }};
}

const std::vector<const RangeByKeySpecTestCase> RangeByKeySpecTestCases() {
  static auto limit1 =
      std::unique_ptr<Skiplist<std::string>::SkiplistLimitSpec>(
          new Skiplist<std::string>::SkiplistLimitSpec({
              .offset = 0,
              .count = -1,
          }));
  static auto limit2 =
      std::unique_ptr<Skiplist<std::string>::SkiplistLimitSpec>(
          new Skiplist<std::string>::SkiplistLimitSpec({
              .offset = 0,
              .count = 2,
          }));
  static auto limit3 =
      std::unique_ptr<Skiplist<std::string>::SkiplistLimitSpec>(
          new Skiplist<std::string>::SkiplistLimitSpec({
              .offset = 2,
              .count = 3,
          }));
  static auto limit4 =
      std::unique_ptr<Skiplist<std::string>::SkiplistLimitSpec>(
          new Skiplist<std::string>::SkiplistLimitSpec({
              .offset = 10,
              .count = -1,
          }));

  return {
      {
          /* base */
          .spec = Skiplist<std::string>::SkiplistRangeByKeySpec(
              "a", false, "zzzzzzz", false, nullptr),
          .keys = {"key0", "key2", "key4", "key5"},
          .revkeys = {"key5", "key4", "key2", "key0"},
          .count = 4,
      },
      {
          /* min exclusive, min is existing key */
          .spec = Skiplist<std::string>::SkiplistRangeByKeySpec(
              "key2", true, "key5", false, nullptr),
          .keys = {"key4", "key5"},
          .revkeys = {"key5", "key4"},
          .count = 2,
      },
      {
          /* min exclusive, min is not existing key */
          .spec = Skiplist<std::string>::SkiplistRangeByKeySpec(
              "key1", true, "key5", false, nullptr),
          .keys = {"key2", "key4", "key5"},
          .revkeys = {"key5", "key4", "key2"},
          .count = 3,
      },
      {
          /* max exclusive, max is existing key */
          .spec = Skiplist<std::string>::SkiplistRangeByKeySpec(
              "key2", false, "key5", true, nullptr),
          .keys = {"key2", "key4"},
          .revkeys = {"key4", "key2"},
          .count = 2,
      },
      {
          /* max exclusive, max is not existing key */
          .spec = Skiplist<std::string>::SkiplistRangeByKeySpec(
              "key1", false, "key6", true, nullptr),
          .keys = {"key2", "key4", "key5"},
          .revkeys = {"key5", "key4", "key2"},
          .count = 3,
      },
      {
          /* count = -1, return all keys in the range */
          .spec = Skiplist<std::string>::SkiplistRangeByKeySpec(
              "key2", false, "key5", false, limit1.get()),
          .keys = {"key2", "key4", "key5"},
          .revkeys = {"key5", "key4", "key2"},
          .count = 3,
      },
      {
          /* with count */
          .spec = Skiplist<std::string>::SkiplistRangeByKeySpec(
              "a", false, "zzzzzzz", false, limit2.get()),
          .keys = {"key0", "key2"},
          .revkeys = {"key5", "key4"},
          .count = 2,
      },
      {
          /* with offset */
          .spec = Skiplist<std::string>::SkiplistRangeByKeySpec(
              "a", false, "zzzzzzz", false, limit3.get()),
          .keys = {"key4", "key5"},
          .revkeys = {"key2", "key0"},
          .count = 2,
      },
      {
          /* offset out of range */
          .spec = Skiplist<std::string>::SkiplistRangeByKeySpec(
              "a", false, "zzzzzzz", false, limit4.get()),
          .keys = {},
          .revkeys = {},
          .count = 0,
      },
      {
          /* invalid spec, non-exclusive, min > max */
          .spec = Skiplist<std::string>::SkiplistRangeByKeySpec(
              "zzzzzzz", false, "a", false, nullptr),
          .keys = {},
          .revkeys = {},
          .count = -1,
      },
      {
          /* invalid spec, max exclusive, min == max */
          .spec = Skiplist<std::string>::SkiplistRangeByKeySpec(
              "key0", false, "key0", true, nullptr),
          .keys = {},
          .revkeys = {},
          .count = -1,
      },
  };
}

TEST_F(SkiplistTest, Iteration) {
  typename Skiplist<std::string>::Iterator it(skiplist);
  it.SeekToLast();
  ASSERT_EQ(*it, "key5");

  --it;
  ASSERT_EQ(*it, "key4");

  ++it;
  ASSERT_EQ(*it, "key5");

  it.SeekToFirst();
  ASSERT_EQ(*it, "key0");

  ++it;
  ASSERT_EQ(*it, "key2");

  ScanSkiplist(skiplist);
}

void ScanSkiplist(const Skiplist<std::string>* skiplist) {
  printf("----start scanning skiplist----\n");
  for (auto it = skiplist->Begin(); it != skiplist->End(); ++it) {
    printf("%s\n", (*it).c_str());
  }
  printf("----end----\n");

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
    skiplist = new Skiplist<std::string, Comparator>(4, cmp);
  }
  static void TearDownTestSuite() {
    delete skiplist;
    skiplist = nullptr;
  }
  static Skiplist<std::string, Comparator>* skiplist;
};

Skiplist<std::string, Comparator>* CustomSkiplistTest::skiplist;

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
