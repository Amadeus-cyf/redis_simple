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
  /* base */
  Skiplist<std::string>::SkiplistRangeByIndexSpec spec1 = {
      .min = 0, .max = 3, .minex = false, .maxex = false, .option = nullptr};
  const std::vector<std::string>& k1 = skiplist->rangeByIndex(&spec1);
  ASSERT_EQ(k1.size(), 4);
  ASSERT_EQ(k1[0], "key0");
  ASSERT_EQ(k1[1], "key2");
  ASSERT_EQ(k1[2], "key4");
  ASSERT_EQ(k1[3], "key5");

  /* min exclusive */
  Skiplist<std::string>::SkiplistRangeByIndexSpec spec2 = {
      .min = 1, .max = 3, .minex = true, .maxex = false, .option = nullptr};
  const std::vector<std::string>& k2 = skiplist->rangeByIndex(&spec2);
  ASSERT_EQ(k2.size(), 2);
  ASSERT_EQ(k2[0], "key4");
  ASSERT_EQ(k2[1], "key5");

  /* max exclusive */
  Skiplist<std::string>::SkiplistRangeByIndexSpec spec3 = {
      .min = 1, .max = 3, .minex = false, .maxex = true, .option = nullptr};
  const std::vector<std::string>& k3 = skiplist->rangeByIndex(&spec3);
  ASSERT_EQ(k3.size(), 2);
  ASSERT_EQ(k3[0], "key2");
  ASSERT_EQ(k3[1], "key4");

  /* with limit */
  const Skiplist<std::string>::SkiplistRangeOption& opt1 =
      Skiplist<std::string>::SkiplistRangeOption{
          .limit = 2,
          .offset = 0,
      };
  Skiplist<std::string>::SkiplistRangeByIndexSpec spec4 = {
      .min = 1,
      .max = 3,
      .minex = false,
      .maxex = false,
      .option = &opt1,
  };
  const std::vector<std::string> k4 = skiplist->rangeByIndex(&spec4);
  ASSERT_EQ(k4.size(), 2);
  ASSERT_EQ(k4[0], "key2");
  ASSERT_EQ(k4[1], "key4");

  /* with offset */
  const Skiplist<std::string>::SkiplistRangeOption& opt2 =
      Skiplist<std::string>::SkiplistRangeOption{
          .limit = -1,
          .offset = 2,
      };
  Skiplist<std::string>::SkiplistRangeByIndexSpec spec5 = {
      .min = 1,
      .max = 3,
      .minex = false,
      .maxex = false,
      .option = &opt2,
  };
  const std::vector<std::string>& k5 = skiplist->rangeByIndex(&spec5);
  ASSERT_EQ(k5.size(), 1);
  ASSERT_EQ(k5[0], "key5");
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
