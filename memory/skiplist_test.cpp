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

struct RangeByIndexSpecTestCase {
  const Skiplist<std::string>::SkiplistRangeByIndexSpec spec;
  const std::vector<std::string> keys;
  const int count;
};

auto opt1 = std::unique_ptr<Skiplist<std::string>::SkiplistRangeOption>(
    new Skiplist<std::string>::SkiplistRangeOption({
        .limit = -1,
        .offset = 0,
    }));
auto opt2 = std::unique_ptr<Skiplist<std::string>::SkiplistRangeOption>(
    new Skiplist<std::string>::SkiplistRangeOption({
        .limit = 2,
        .offset = 0,
    }));
auto opt3 = std::unique_ptr<Skiplist<std::string>::SkiplistRangeOption>(
    new Skiplist<std::string>::SkiplistRangeOption({
        .limit = 3,
        .offset = 2,
    }));
auto opt4 = std::unique_ptr<Skiplist<std::string>::SkiplistRangeOption>(
    new Skiplist<std::string>::SkiplistRangeOption({
        .limit = -1,
        .offset = 10,
    }));

const std::vector<RangeByIndexSpecTestCase> rangeByIndexSpecTestCases();
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

TEST_F(SkiplistTest, GetElementByRank) {
  const std::string& s0 = skiplist->getElementByRank(0);
  ASSERT_EQ(s0, "key0");

  const std::string& s1 = skiplist->getElementByRank(1);
  ASSERT_EQ(s1, "key2");

  const std::string& s2 = skiplist->getElementByRank(-1);
  ASSERT_EQ(s2, "key5");

  const std::string& s3 = skiplist->getElementByRank(-2);
  ASSERT_EQ(s3, "key4");

  const std::string& s4 = skiplist->getElementByRank(-4);
  ASSERT_EQ(s4, "key0");

  ASSERT_THROW(skiplist->getElementByRank(skiplist->size()), std::out_of_range);
  ASSERT_THROW(skiplist->getElementByRank(-skiplist->size() - 1),
               std::out_of_range);
  ASSERT_THROW(skiplist->getElementByRank(INT_MAX), std::out_of_range);
  ASSERT_THROW(skiplist->getElementByRank(INT_MIN), std::out_of_range);
}

TEST_F(SkiplistTest, GetRankofElement) {
  ssize_t r0 = skiplist->getRankofElement("key0");
  ASSERT_EQ(r0, 0);

  ssize_t r1 = skiplist->getRankofElement("key2");
  ASSERT_EQ(r1, 1);

  ssize_t r2 = skiplist->getRankofElement("key5");
  ASSERT_EQ(r2, 3);

  ssize_t r3 = skiplist->getRankofElement("key_not_exist");
  ASSERT_EQ(r3, -1);
}

TEST_F(SkiplistTest, GetElementsGt) {
  const std::vector<std::string>& k0 = skiplist->getElementsGt("key0");
  ASSERT_EQ(k0.size(), 3);
  ASSERT_EQ(k0[0], "key2");

  const std::vector<std::string>& k1 = skiplist->getElementsGt("key3");
  ASSERT_EQ(k1.size(), 2);
  ASSERT_EQ(k1[0], "key4");

  const std::vector<std::string>& k2 = skiplist->getElementsGt("key6");
  ASSERT_EQ(k2.size(), 0);

  const std::vector<std::string>& k3 = skiplist->getElementsGt("abc");
  ASSERT_EQ(k3.size(), 4);
  ASSERT_EQ(k3[0], "key0");
}

TEST_F(SkiplistTest, GetElementsGte) {
  const std::vector<std::string>& k0 = skiplist->getElementsGte("key0");
  ASSERT_EQ(k0.size(), 4);
  ASSERT_EQ(k0[0], "key0");

  const std::vector<std::string>& k1 = skiplist->getElementsGte("key3");
  ASSERT_EQ(k1.size(), 2);
  ASSERT_EQ(k1[0], "key4");

  const std::vector<std::string>& k2 = skiplist->getElementsGte("key6");
  ASSERT_EQ(k2.size(), 0);

  const std::vector<std::string>& k3 = skiplist->getElementsGte("key5");
  ASSERT_EQ(k3.size(), 1);
  ASSERT_EQ(k3[0], "key5");
}

TEST_F(SkiplistTest, GetElementsLt) {
  const std::vector<std::string>& k0 = skiplist->getElementsLt("key5");
  ASSERT_EQ(k0.size(), 3);
  ASSERT_EQ(k0[0], "key0");
  ASSERT_EQ(k0[2], "key4");

  const std::vector<std::string>& k1 = skiplist->getElementsLt("key3");
  ASSERT_EQ(k1.size(), 2);
  ASSERT_EQ(k1[0], "key0");
  ASSERT_EQ(k1[1], "key2");

  const std::vector<std::string>& k2 = skiplist->getElementsLt("key2");
  ASSERT_EQ(k2.size(), 1);
  ASSERT_EQ(k2[0], "key0");

  const std::vector<std::string>& k3 = skiplist->getElementsLt("key0");
  ASSERT_EQ(k3.size(), 0);

  const std::vector<std::string>& k4 = skiplist->getElementsLt("key6");
  ASSERT_EQ(k4.size(), 4);
  ASSERT_EQ(k4[0], "key0");
  ASSERT_EQ(k4[3], "key5");
}

TEST_F(SkiplistTest, GetElementsLte) {
  const std::vector<std::string>& k0 = skiplist->getElementsLte("key5");
  ASSERT_EQ(k0.size(), 4);
  ASSERT_EQ(k0[0], "key0");
  ASSERT_EQ(k0[3], "key5");

  const std::vector<std::string>& k1 = skiplist->getElementsLte("key3");
  ASSERT_EQ(k1.size(), 2);
  ASSERT_EQ(k1[0], "key0");
  ASSERT_EQ(k1[1], "key2");

  const std::vector<std::string>& k2 = skiplist->getElementsLte("key2");
  ASSERT_EQ(k2.size(), 2);
  ASSERT_EQ(k2[0], "key0");
  ASSERT_EQ(k2[1], "key2");

  const std::vector<std::string>& k3 = skiplist->getElementsLte("key0");
  ASSERT_EQ(k3.size(), 1);
  ASSERT_EQ(k3[0], "key0");

  const std::vector<std::string>& k4 = skiplist->getElementsLte("abc");
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

TEST_F(SkiplistTest, RangeByIndex) {
  const std::vector<RangeByIndexSpecTestCase>& tests =
      rangeByIndexSpecTestCases();
  for (const RangeByIndexSpecTestCase& test : tests) {
    const std::vector<std::string>& keys = skiplist->rangeByIndex(&test.spec);
    ASSERT_EQ(keys.size(), test.keys.size());
    for (int i = 0; i < keys.size(); ++i) {
      ASSERT_EQ(keys[i], test.keys[i]);
    }
  }
}

TEST_F(SkiplistTest, RangeByIndexCount) {
  const std::vector<RangeByIndexSpecTestCase>& tests =
      rangeByIndexSpecTestCases();
  for (const RangeByIndexSpecTestCase& test : tests) {
    int count = skiplist->rangeByIndexCount(&test.spec);
    ASSERT_EQ(count, test.count);
  }
}

const std::vector<RangeByIndexSpecTestCase> rangeByIndexSpecTestCases() {
  return {
      {
          /* base */
          .spec = {.min = 0,
                   .max = 3,
                   .minex = false,
                   .maxex = false,
                   .option = nullptr},
          .keys = {"key0", "key2", "key4", "key5"},
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
          .count = 2,
      },
      {
          /* with offset */
          .spec =
              {
                  .min = 1,
                  .max = 3,
                  .minex = false,
                  .maxex = false,
                  .option = opt3.get(),
              },
          .keys = {"key5"},
          .count = 1,
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
          .count = -1,
      },
      {
          /* max exclusive, min == max */
          .spec =
              {
                  .min = 1,
                  .max = 1,
                  .minex = false,
                  .maxex = true,
                  .option = nullptr,
              },
          .keys = {},
          .count = -1,
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
          .count = 0,
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
