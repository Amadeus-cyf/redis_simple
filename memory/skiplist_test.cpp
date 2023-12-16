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
  const int count;
};

const std::vector<const RangeByRankSpecTestCase> rangeByRankSpecTestCases();
const std::vector<const RangeByKeySpecTestCase> rangeByKeySpecTestCases();
void scanSkiplist(const Skiplist<std::string>* skiplist);

Skiplist<std::string>* SkiplistTest::skiplist;

TEST_F(SkiplistTest, Insertion) {
  ASSERT_EQ(skiplist->insert("key1"), "key1");
  ASSERT_EQ(skiplist->insert("key2"), "key2");
  ASSERT_EQ(skiplist->insert("key0"), "key0");
  ASSERT_EQ(skiplist->insert("key1"), "key1");
  ASSERT_EQ(skiplist->size(), 3);

  ASSERT_TRUE(skiplist->contains("key1"));
  ASSERT_TRUE(skiplist->contains("key2"));
  ASSERT_TRUE(skiplist->contains("key0"));
  ASSERT_TRUE(!skiplist->contains("key_not_exist"));
}

TEST_F(SkiplistTest, Deletion) {
  ASSERT_TRUE(skiplist->del("key1"));
  ASSERT_FALSE(skiplist->contains("key1"));
  ASSERT_EQ(skiplist->size(), 2);
  ASSERT_FALSE(skiplist->del("key1"));
  ASSERT_EQ(skiplist->size(), 2);

  ASSERT_TRUE(skiplist->del("key2"));
  ASSERT_FALSE(skiplist->contains("key2"));
  ASSERT_EQ(skiplist->size(), 1);
  ASSERT_FALSE(skiplist->del("key2"));
  ASSERT_EQ(skiplist->size(), 1);

  ASSERT_FALSE(skiplist->del("key_not_exist"));
}

TEST_F(SkiplistTest, Update) {
  ASSERT_EQ("key1", skiplist->insert("key1"));
  ASSERT_EQ("key2", skiplist->insert("key2"));
  ASSERT_EQ("key3", skiplist->insert("key3"));
  ASSERT_EQ(skiplist->size(), 4);

  ASSERT_TRUE(skiplist->update("key3", "key5"));
  ASSERT_EQ(skiplist->size(), 4);
  ASSERT_FALSE(skiplist->contains("key3"));
  ASSERT_TRUE(skiplist->contains("key5"));

  ASSERT_TRUE(skiplist->update("key1", "key4"));

  ASSERT_EQ(skiplist->size(), 4);
  ASSERT_FALSE(skiplist->contains("key1"));
  ASSERT_TRUE(skiplist->contains("key4"));

  ASSERT_TRUE(!skiplist->update("key_not_exist", "key6"));
  ASSERT_EQ(skiplist->size(), 4);
}

TEST_F(SkiplistTest, GetKeyByRank) {
  const std::string& s0 = skiplist->getKeyByRank(0);
  ASSERT_EQ(s0, "key0");

  const std::string& s1 = skiplist->getKeyByRank(1);
  ASSERT_EQ(s1, "key2");

  const std::string& s2 = skiplist->getKeyByRank(-1);
  ASSERT_EQ(s2, "key5");

  const std::string& s3 = skiplist->getKeyByRank(-2);
  ASSERT_EQ(s3, "key4");

  const std::string& s4 = skiplist->getKeyByRank(-4);
  ASSERT_EQ(s4, "key0");

  ASSERT_THROW(skiplist->getKeyByRank(skiplist->size()), std::out_of_range);
  ASSERT_THROW(skiplist->getKeyByRank(-skiplist->size() - 1),
               std::out_of_range);
  ASSERT_THROW(skiplist->getKeyByRank(INT_MAX), std::out_of_range);
  ASSERT_THROW(skiplist->getKeyByRank(INT_MIN), std::out_of_range);
}

TEST_F(SkiplistTest, GetRankofKey) {
  ssize_t r0 = skiplist->getRankofKey("key0");
  ASSERT_EQ(r0, 0);

  ssize_t r1 = skiplist->getRankofKey("key2");
  ASSERT_EQ(r1, 1);

  ssize_t r2 = skiplist->getRankofKey("key5");
  ASSERT_EQ(r2, 3);

  ssize_t r3 = skiplist->getRankofKey("key_not_exist");
  ASSERT_EQ(r3, -1);
}

TEST_F(SkiplistTest, getKeysGt) {
  const std::vector<std::string>& k0 = skiplist->getKeysGt("key0");
  ASSERT_EQ(k0.size(), 3);
  ASSERT_EQ(k0[0], "key2");

  const std::vector<std::string>& k1 = skiplist->getKeysGt("key3");
  ASSERT_EQ(k1.size(), 2);
  ASSERT_EQ(k1[0], "key4");

  const std::vector<std::string>& k2 = skiplist->getKeysGt("key6");
  ASSERT_EQ(k2.size(), 0);

  const std::vector<std::string>& k3 = skiplist->getKeysGt("abc");
  ASSERT_EQ(k3.size(), 4);
  ASSERT_EQ(k3[0], "key0");
}

TEST_F(SkiplistTest, getKeysGte) {
  const std::vector<std::string>& k0 = skiplist->getKeysGte("key0");
  ASSERT_EQ(k0.size(), 4);
  ASSERT_EQ(k0[0], "key0");

  const std::vector<std::string>& k1 = skiplist->getKeysGte("key3");
  ASSERT_EQ(k1.size(), 2);
  ASSERT_EQ(k1[0], "key4");

  const std::vector<std::string>& k2 = skiplist->getKeysGte("key6");
  ASSERT_EQ(k2.size(), 0);

  const std::vector<std::string>& k3 = skiplist->getKeysGte("key5");
  ASSERT_EQ(k3.size(), 1);
  ASSERT_EQ(k3[0], "key5");
}

TEST_F(SkiplistTest, getKeysLt) {
  const std::vector<std::string>& k0 = skiplist->getKeysLt("key5");
  ASSERT_EQ(k0.size(), 3);
  ASSERT_EQ(k0[0], "key0");
  ASSERT_EQ(k0[2], "key4");

  const std::vector<std::string>& k1 = skiplist->getKeysLt("key3");
  ASSERT_EQ(k1.size(), 2);
  ASSERT_EQ(k1[0], "key0");
  ASSERT_EQ(k1[1], "key2");

  const std::vector<std::string>& k2 = skiplist->getKeysLt("key2");
  ASSERT_EQ(k2.size(), 1);
  ASSERT_EQ(k2[0], "key0");

  const std::vector<std::string>& k3 = skiplist->getKeysLt("key0");
  ASSERT_EQ(k3.size(), 0);

  const std::vector<std::string>& k4 = skiplist->getKeysLt("key6");
  ASSERT_EQ(k4.size(), 4);
  ASSERT_EQ(k4[0], "key0");
  ASSERT_EQ(k4[3], "key5");
}

TEST_F(SkiplistTest, getKeysLte) {
  const std::vector<std::string>& k0 = skiplist->getKeysLte("key5");
  ASSERT_EQ(k0.size(), 4);
  ASSERT_EQ(k0[0], "key0");
  ASSERT_EQ(k0[3], "key5");

  const std::vector<std::string>& k1 = skiplist->getKeysLte("key3");
  ASSERT_EQ(k1.size(), 2);
  ASSERT_EQ(k1[0], "key0");
  ASSERT_EQ(k1[1], "key2");

  const std::vector<std::string>& k2 = skiplist->getKeysLte("key2");
  ASSERT_EQ(k2.size(), 2);
  ASSERT_EQ(k2[0], "key0");
  ASSERT_EQ(k2[1], "key2");

  const std::vector<std::string>& k3 = skiplist->getKeysLte("key0");
  ASSERT_EQ(k3.size(), 1);
  ASSERT_EQ(k3[0], "key0");

  const std::vector<std::string>& k4 = skiplist->getKeysLte("abc");
  ASSERT_EQ(k4.size(), 0);
}

TEST_F(SkiplistTest, ArrayAccess) {
  ASSERT_EQ((*skiplist)[0], "key0");
  ASSERT_EQ((*skiplist)[1], "key2");
  ASSERT_EQ((*skiplist)[2], "key4");
  ASSERT_EQ((*skiplist)[3], "key5");
  ASSERT_EQ((*skiplist)[skiplist->size() - 1], "key5");

  ASSERT_THROW((*skiplist)[skiplist->size()], std::out_of_range);
  ASSERT_THROW((*skiplist)[-1], std::out_of_range);
  ASSERT_THROW((*skiplist)[INT_MAX], std::out_of_range);
  ASSERT_THROW((*skiplist)[INT_MIN], std::out_of_range);
}

TEST_F(SkiplistTest, RangeByRank) {
  const std::vector<const RangeByRankSpecTestCase>& tests =
      rangeByRankSpecTestCases();
  for (const RangeByRankSpecTestCase& test : tests) {
    ASSERT_EQ(skiplist->rangeByRank(&test.spec), test.keys);
  }
}

TEST_F(SkiplistTest, rangeCount) {
  const std::vector<const RangeByRankSpecTestCase>& tests =
      rangeByRankSpecTestCases();
  for (const RangeByRankSpecTestCase& test : tests) {
    ASSERT_EQ(skiplist->rangeCount(&test.spec), test.count);
  }
}

TEST_F(SkiplistTest, RevRangeByRank) {
  const std::vector<const RangeByRankSpecTestCase>& tests =
      rangeByRankSpecTestCases();
  for (const RangeByRankSpecTestCase& test : tests) {
    ASSERT_EQ(skiplist->revRangeByRank(&test.spec), test.revkeys);
  }
}

TEST_F(SkiplistTest, RangeByKey) {
  const std::vector<const RangeByKeySpecTestCase>& tests =
      rangeByKeySpecTestCases();
  for (const RangeByKeySpecTestCase& test : tests) {
    const std::vector<const RangeByKeySpecTestCase>& tests =
        rangeByKeySpecTestCases();
    for (const RangeByKeySpecTestCase& test : tests) {
      ASSERT_EQ(skiplist->rangeByKey(&test.spec), test.keys);
    }
  }
}

TEST_F(SkiplistTest, RevRangeByKey) {
  const std::vector<const RangeByKeySpecTestCase>& tests =
      rangeByKeySpecTestCases();
  for (const RangeByKeySpecTestCase& test : tests) {
    const std::vector<const RangeByKeySpecTestCase>& tests =
        rangeByKeySpecTestCases();
    for (const RangeByKeySpecTestCase& test : tests) {
      ASSERT_EQ(skiplist->revRangeByKey(&test.spec), test.revkeys);
    }
  }
}

const std::vector<const RangeByRankSpecTestCase> rangeByRankSpecTestCases() {
  static auto opt1 =
      std::unique_ptr<Skiplist<std::string>::SkiplistRangeOption>(
          new Skiplist<std::string>::SkiplistRangeOption({
              .limit = -1,
              .offset = 0,
          }));
  static auto opt2 =
      std::unique_ptr<Skiplist<std::string>::SkiplistRangeOption>(
          new Skiplist<std::string>::SkiplistRangeOption({
              .limit = 2,
              .offset = 0,
          }));
  static auto opt3 =
      std::unique_ptr<Skiplist<std::string>::SkiplistRangeOption>(
          new Skiplist<std::string>::SkiplistRangeOption({
              .limit = 3,
              .offset = 2,
          }));
  static auto opt4 =
      std::unique_ptr<Skiplist<std::string>::SkiplistRangeOption>(
          new Skiplist<std::string>::SkiplistRangeOption({
              .limit = -1,
              .offset = 10,
          }));

  return {
      {
          /* base */
          .spec = {.min = 0,
                   .max = 3,
                   .minex = false,
                   .maxex = false,
                   .option = nullptr},
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
                   .option = nullptr},
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
                   .option = nullptr},
          .keys = {"key2", "key4"},
          .revkeys = {"key4", "key2"},
          .count = 2,
      },
      {
          /* limit = -1, return all keys in the range */
          .spec =
              {
                  .min = 1,
                  .max = 3,
                  .minex = false,
                  .maxex = false,
                  .option = opt1.get(),
              },
          .keys = {"key2", "key4", "key5"},
          .revkeys = {"key4", "key2", "key0"},
          .count = 3,
      },
      {
          /* with limit */
          .spec =
              {
                  .min = 1,
                  .max = 3,
                  .minex = false,
                  .maxex = false,
                  .option = opt2.get(),
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
                  .option = opt3.get(),
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
                  .option = opt4.get(),
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
                  .option = nullptr,
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
                  .option = nullptr,
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
                  .option = nullptr,
              },
          .keys = {},
          .revkeys = {},
          .count = -1,
      },
  };
}

const std::vector<const RangeByKeySpecTestCase> rangeByKeySpecTestCases() {
  static auto opt1 =
      std::unique_ptr<Skiplist<std::string>::SkiplistRangeOption>(
          new Skiplist<std::string>::SkiplistRangeOption({
              .limit = -1,
              .offset = 0,
          }));
  static auto opt2 =
      std::unique_ptr<Skiplist<std::string>::SkiplistRangeOption>(
          new Skiplist<std::string>::SkiplistRangeOption({
              .limit = 2,
              .offset = 0,
          }));
  static auto opt3 =
      std::unique_ptr<Skiplist<std::string>::SkiplistRangeOption>(
          new Skiplist<std::string>::SkiplistRangeOption({
              .limit = 3,
              .offset = 2,
          }));
  static auto opt4 =
      std::unique_ptr<Skiplist<std::string>::SkiplistRangeOption>(
          new Skiplist<std::string>::SkiplistRangeOption({
              .limit = -1,
              .offset = 10,
          }));

  return {
      {
          /* base */
          .spec =
              {
                  .min = "a",
                  .max = "zzzzzzzz",
                  .minex = false,
                  .maxex = false,
                  .option = nullptr,
              },
          .keys = {"key0", "key2", "key4", "key5"},
          .revkeys = {"key5", "key4", "key2", "key0"},
          .count = 4,
      },
      {
          /* min exclusive */
          .spec = {.min = "key2",
                   .max = "key5",
                   .minex = true,
                   .maxex = false,
                   .option = nullptr},
          .keys = {"key4", "key5"},
          .revkeys = {"key5", "key4"},
          .count = 2,
      },
      {
          /* max exclusive */
          .spec = {.min = "key2",
                   .max = "key5",
                   .minex = false,
                   .maxex = true,
                   .option = nullptr},
          .keys = {"key2", "key4"},
          .revkeys = {"key4", "key2"},
          .count = 2,
      },
      {
          /* limit = -1, return all keys in the range */
          .spec =
              {
                  .min = "key2",
                  .max = "key5",
                  .minex = false,
                  .maxex = false,
                  .option = opt1.get(),
              },
          .keys = {"key2", "key4", "key5"},
          .revkeys = {"key5", "key4", "key2"},
          .count = 3,
      },
      {
          /* with limit */
          .spec =
              {
                  .min = "a",
                  .max = "zzzzzzz",
                  .minex = false,
                  .maxex = false,
                  .option = opt2.get(),
              },
          .keys = {"key0", "key2"},
          .revkeys = {"key5", "key4"},
          .count = 2,
      },
      {
          /* with offset */
          .spec =
              {
                  .min = "a",
                  .max = "zzzzzzz",
                  .minex = false,
                  .maxex = false,
                  .option = opt3.get(),
              },
          .keys = {"key4", "key5"},
          .revkeys = {"key2", "key0"},
          .count = 2,
      },
      {
          /* offset out of range */
          .spec =
              {
                  .min = "a",
                  .max = "zzzzzzz",
                  .minex = false,
                  .maxex = false,
                  .option = opt4.get(),
              },
          .keys = {},
          .revkeys = {},
          .count = 0,
      },
      {
          /* invalid spec, non-exclusive, min > max */
          .spec =
              {
                  .min = "zzzzzzz",
                  .max = "a",
                  .minex = false,
                  .maxex = false,
                  .option = nullptr,
              },
          .keys = {},
          .revkeys = {},
          .count = -1,
      },
      {
          /* invalid spec, max exclusive, min == max */
          .spec =
              {
                  .min = "key0",
                  .max = "key0",
                  .minex = false,
                  .maxex = true,
                  .option = nullptr,
              },
          .keys = {},
          .revkeys = {},
          .count = -1,
      },
  };
}

TEST_F(SkiplistTest, Iteration) {
  typename Skiplist<std::string>::Iterator it(skiplist);
  it.seekToLast();
  ASSERT_EQ(*it, "key5");

  --it;
  ASSERT_EQ(*it, "key4");

  ++it;
  ASSERT_EQ(*it, "key5");

  it.seekToFirst();
  ASSERT_EQ(*it, "key0");

  ++it;
  ASSERT_EQ(*it, "key2");

  scanSkiplist(skiplist);
}

void scanSkiplist(const Skiplist<std::string>* skiplist) {
  printf("----start scanning skiplist----\n");
  for (auto it = skiplist->begin(); it != skiplist->end(); ++it) {
    printf("%s\n", (*it).c_str());
  }
  printf("----end----\n");

  skiplist->print();
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
  ASSERT_EQ(skiplist->insert("key1"), "key1");
  ASSERT_EQ(skiplist->insert("key2"), "key2");
  ASSERT_EQ(skiplist->insert("key0"), "key0");
  ASSERT_EQ(skiplist->insert("key1"), "key1");
  ASSERT_EQ(skiplist->size(), 3);

  ASSERT_TRUE(skiplist->contains("key1"));
  ASSERT_TRUE(skiplist->contains("key2"));
  ASSERT_TRUE(skiplist->contains("key0"));
  ASSERT_TRUE(!skiplist->contains("key_not_exist"));
}

TEST_F(CustomSkiplistTest, ArrayAccess) {
  ASSERT_EQ((*skiplist)[0], "key2");
  ASSERT_EQ((*skiplist)[1], "key1");
  ASSERT_EQ((*skiplist)[2], "key0");
  ASSERT_EQ((*skiplist)[skiplist->size() - 1], "key0");

  ASSERT_THROW((*skiplist)[skiplist->size()], std::out_of_range);
  ASSERT_THROW((*skiplist)[-1], std::out_of_range);
  ASSERT_THROW((*skiplist)[INT_MAX], std::out_of_range);
  ASSERT_THROW((*skiplist)[INT_MIN], std::out_of_range);
}
}  // namespace in_memory
}  // namespace redis_simple
