#include "memory/skiplist.h"

#include <gtest/gtest.h>

#include <string>

namespace skiplist {

void scanSkiplist(const Skiplist<std::string>& skiplist);

Skiplist<std::string> skiplist(4);

TEST(SkiplistTest, Insertion) {
  ASSERT_TRUE(skiplist.insert("key1"));
  ASSERT_TRUE(skiplist.insert("key2"));
  ASSERT_TRUE(skiplist.insert("key0"));
  ASSERT_FALSE(skiplist.insert("key1"));
  ASSERT_EQ(skiplist.size(), 3);

  ASSERT_TRUE(skiplist.contains("key1"));
  ASSERT_TRUE(skiplist.contains("key2"));
  ASSERT_TRUE(skiplist.contains("key0"));
  ASSERT_TRUE(!skiplist.contains("key_not_exist"));
}

TEST(SkiplistTest, Deletion) {
  ASSERT_TRUE(skiplist.del("key1"));
  ASSERT_FALSE(skiplist.contains("key1"));
  ASSERT_EQ(skiplist.size(), 2);
  ASSERT_FALSE(skiplist.del("key1"));
  ASSERT_EQ(skiplist.size(), 2);

  ASSERT_TRUE(skiplist.del("key2"));
  ASSERT_FALSE(skiplist.contains("key2"));
  ASSERT_EQ(skiplist.size(), 1);
  ASSERT_FALSE(skiplist.del("key2"));
  ASSERT_EQ(skiplist.size(), 1);

  ASSERT_FALSE(skiplist.del("key_not_exist"));
}

TEST(SkiplistTest, Update) {
  ASSERT_TRUE(skiplist.insert("key1"));
  ASSERT_TRUE(skiplist.insert("key2"));
  ASSERT_TRUE(skiplist.insert("key3"));
  ASSERT_EQ(skiplist.size(), 4);

  ASSERT_TRUE(skiplist.update("key3", "key5"));
  ASSERT_EQ(skiplist.size(), 4);
  ASSERT_FALSE(skiplist.contains("key3"));
  ASSERT_TRUE(skiplist.contains("key5"));

  ASSERT_TRUE(skiplist.update("key1", "key4"));

  ASSERT_EQ(skiplist.size(), 4);
  ASSERT_FALSE(skiplist.contains("key1"));
  ASSERT_TRUE(skiplist.contains("key4"));

  ASSERT_TRUE(!skiplist.update("key_not_exist", "key6"));
  ASSERT_EQ(skiplist.size(), 4);
}

TEST(SkiplistTest, GetElementByRank) {
  const std::string& s0 = skiplist.getElementByRank(0);
  ASSERT_EQ(s0, "key0");

  const std::string& s1 = skiplist.getElementByRank(1);
  ASSERT_EQ(s1, "key2");

  const std::string& s2 = skiplist.getElementByRank(-1);
  ASSERT_EQ(s2, "key5");

  const std::string& s3 = skiplist.getElementByRank(-2);
  ASSERT_EQ(s3, "key4");

  const std::string& s4 = skiplist.getElementByRank(-4);
  ASSERT_EQ(s4, "key0");

  ASSERT_THROW(skiplist.getElementByRank(skiplist.size()), std::out_of_range);
  ASSERT_THROW(skiplist.getElementByRank(-skiplist.size() - 1),
               std::out_of_range);
  ASSERT_THROW(skiplist.getElementByRank(INT_MAX), std::out_of_range);
  ASSERT_THROW(skiplist.getElementByRank(INT_MIN), std::out_of_range);
}

TEST(SkiplistTest, GetRankofElement) {
  ssize_t r0 = skiplist.getRankofElement("key0");
  ASSERT_EQ(r0, 0);

  ssize_t r1 = skiplist.getRankofElement("key2");
  ASSERT_EQ(r1, 1);

  ssize_t r2 = skiplist.getRankofElement("key5");
  ASSERT_EQ(r2, 3);

  ssize_t r3 = skiplist.getRankofElement("key_not_exist");
  ASSERT_EQ(r3, -1);
}

TEST(SkiplistTest, GetElementsByRange) {
  const std::vector<std::string>& k1 = skiplist.getElementsByRange(0, 4);
  ASSERT_EQ(k1.size(), 4);
  ASSERT_EQ(k1[0], "key0");
  ASSERT_EQ(k1[3], "key5");

  const std::vector<std::string>& k2 = skiplist.getElementsByRange(1, 1);
  ASSERT_EQ(k2.size(), 1);
  ASSERT_EQ(k2[0], "key2");

  const std::vector<std::string>& k3 = skiplist.getElementsByRange(3, 0);
  ASSERT_EQ(k3.size(), 0);

  const std::vector<std::string>& k4 = skiplist.getElementsByRange(3, INT_MAX);
  ASSERT_EQ(k4.size(), 1);
  ASSERT_EQ(k4[0], "key5");

  const std::vector<std::string>& k5 = skiplist.getElementsByRange(-1, 1);
  ASSERT_EQ(k5.size(), 1);
  ASSERT_EQ(k5[0], "key5");

  const std::vector<std::string>& k6 = skiplist.getElementsByRange(-2, 2);
  ASSERT_EQ(k6.size(), 2);
  ASSERT_EQ(k6[0], "key4");
  ASSERT_EQ(k6[1], "key5");

  const std::vector<std::string>& k7 = skiplist.getElementsByRange(-2, INT_MAX);
  ASSERT_EQ(k7.size(), 2);
  ASSERT_EQ(k7[0], "key4");
  ASSERT_EQ(k7[1], "key5");

  const std::vector<std::string>& k8 = skiplist.getElementsByRange(INT_MAX, 1);
  ASSERT_EQ(k8.size(), 0);

  const std::vector<std::string>& k9 = skiplist.getElementsByRange(INT_MIN, 1);
  ASSERT_EQ(k9.size(), 0);
}

TEST(SkiplistTest, GetElementsByRevRange) {
  const std::vector<std::string>& k1 = skiplist.getElementsByRevRange(-1, 4);
  ASSERT_EQ(k1.size(), 4);
  ASSERT_EQ(k1[0], "key5");
  ASSERT_EQ(k1[1], "key4");
  ASSERT_EQ(k1[2], "key2");
  ASSERT_EQ(k1[3], "key0");

  const std::vector<std::string>& k2 = skiplist.getElementsByRevRange(-1, 3);
  ASSERT_EQ(k2.size(), 3);
  ASSERT_EQ(k2[0], "key5");
  ASSERT_EQ(k2[1], "key4");
  ASSERT_EQ(k2[2], "key2");

  const std::vector<std::string>& k3 =
      skiplist.getElementsByRevRange(-1, INT_MAX);
  ASSERT_EQ(k3.size(), 4);
  ASSERT_EQ(k3[0], "key5");
  ASSERT_EQ(k3[3], "key0");

  const std::vector<std::string>& k4 = skiplist.getElementsByRevRange(-1, 1);
  ASSERT_EQ(k4.size(), 1);
  ASSERT_EQ(k4[0], "key5");

  const std::vector<std::string>& k5 = skiplist.getElementsByRevRange(1, 2);
  ASSERT_EQ(k5.size(), 2);
  ASSERT_EQ(k5[0], "key2");
  ASSERT_EQ(k5[1], "key0");

  const std::vector<std::string>& k6 =
      skiplist.getElementsByRevRange(1, INT_MAX);
  ASSERT_EQ(k6.size(), 2);
  ASSERT_EQ(k6[0], "key2");
  ASSERT_EQ(k6[1], "key0");

  const std::vector<std::string>& k7 = skiplist.getElementsByRevRange(0, 1);
  ASSERT_EQ(k7.size(), 1);
  ASSERT_EQ(k7[0], "key0");

  const std::vector<std::string>& k8 =
      skiplist.getElementsByRevRange(INT_MAX, 1);
  ASSERT_EQ(k8.size(), 0);

  const std::vector<std::string>& k9 =
      skiplist.getElementsByRevRange(INT_MIN, 1);
  ASSERT_EQ(k9.size(), 0);
}

TEST(SkiplistTest, GetElementsGt) {
  const std::vector<std::string>& k0 = skiplist.getElementsGt("key0");
  ASSERT_EQ(k0.size(), 3);
  ASSERT_EQ(k0[0], "key2");

  const std::vector<std::string>& k1 = skiplist.getElementsGt("key3");
  ASSERT_EQ(k1.size(), 2);
  ASSERT_EQ(k1[0], "key4");

  const std::vector<std::string>& k2 = skiplist.getElementsGt("key6");
  ASSERT_EQ(k2.size(), 0);

  const std::vector<std::string>& k3 = skiplist.getElementsGt("abc");
  ASSERT_EQ(k3.size(), 4);
  ASSERT_EQ(k3[0], "key0");
}

TEST(SkiplistTest, GetElementsGte) {
  const std::vector<std::string>& k0 = skiplist.getElementsGte("key0");
  ASSERT_EQ(k0.size(), 4);
  ASSERT_EQ(k0[0], "key0");

  const std::vector<std::string>& k1 = skiplist.getElementsGte("key3");
  ASSERT_EQ(k1.size(), 2);
  ASSERT_EQ(k1[0], "key4");

  const std::vector<std::string>& k2 = skiplist.getElementsGte("key6");
  ASSERT_EQ(k2.size(), 0);

  const std::vector<std::string>& k3 = skiplist.getElementsGte("key5");
  ASSERT_EQ(k3.size(), 1);
  ASSERT_EQ(k3[0], "key5");
}

TEST(SkiplistTest, GetElementsLt) {
  const std::vector<std::string>& k0 = skiplist.getElementsLt("key5");
  ASSERT_EQ(k0.size(), 3);
  ASSERT_EQ(k0[0], "key0");
  ASSERT_EQ(k0[2], "key4");

  const std::vector<std::string>& k1 = skiplist.getElementsLt("key3");
  ASSERT_EQ(k1.size(), 2);
  ASSERT_EQ(k1[0], "key0");
  ASSERT_EQ(k1[1], "key2");

  const std::vector<std::string>& k2 = skiplist.getElementsLt("key2");
  ASSERT_EQ(k2.size(), 1);
  ASSERT_EQ(k2[0], "key0");

  const std::vector<std::string>& k3 = skiplist.getElementsLt("key0");
  ASSERT_EQ(k3.size(), 0);

  const std::vector<std::string>& k4 = skiplist.getElementsLt("key6");
  ASSERT_EQ(k4.size(), 4);
  ASSERT_EQ(k4[0], "key0");
  ASSERT_EQ(k4[3], "key5");
}

TEST(SkiplistTest, GetElementsLte) {
  const std::vector<std::string>& k0 = skiplist.getElementsLte("key5");
  ASSERT_EQ(k0.size(), 4);
  ASSERT_EQ(k0[0], "key0");
  ASSERT_EQ(k0[3], "key5");

  const std::vector<std::string>& k1 = skiplist.getElementsLte("key3");
  ASSERT_EQ(k1.size(), 2);
  ASSERT_EQ(k1[0], "key0");
  ASSERT_EQ(k1[1], "key2");

  const std::vector<std::string>& k2 = skiplist.getElementsLte("key2");
  ASSERT_EQ(k2.size(), 2);
  ASSERT_EQ(k2[0], "key0");
  ASSERT_EQ(k2[1], "key2");

  const std::vector<std::string>& k3 = skiplist.getElementsLte("key0");
  ASSERT_EQ(k3.size(), 1);
  ASSERT_EQ(k3[0], "key0");

  const std::vector<std::string>& k4 = skiplist.getElementsLte("abc");
  ASSERT_EQ(k4.size(), 0);
}

TEST(SkiplistTest, GetElementsInRange) {
  const std::vector<std::string>& k0 =
      skiplist.getElementsInRange("key0", "key6");
  ASSERT_EQ(k0.size(), 4);
  ASSERT_EQ(k0[0], "key0");
  ASSERT_EQ(k0[3], "key5");

  const std::vector<std::string>& k1 =
      skiplist.getElementsInRange("key0", "key5");
  ASSERT_EQ(k1.size(), 3);
  ASSERT_EQ(k1[0], "key0");
  ASSERT_EQ(k1[2], "key4");

  const std::vector<std::string>& k2 =
      skiplist.getElementsInRange("key0", "key3");
  ASSERT_EQ(k2.size(), 2);
  ASSERT_EQ(k2[0], "key0");
  ASSERT_EQ(k2[1], "key2");

  const std::vector<std::string>& k3 =
      skiplist.getElementsInRange("key0", "key2");
  ASSERT_EQ(k3.size(), 1);
  ASSERT_EQ(k3[0], "key0");

  const std::vector<std::string>& k4 =
      skiplist.getElementsInRange("key0", "key1");
  ASSERT_EQ(k4.size(), 1);
  ASSERT_EQ(k4[0], "key0");

  const std::vector<std::string>& k5 =
      skiplist.getElementsInRange("key0", "key0");
  ASSERT_EQ(k5.size(), 0);

  const std::vector<std::string>& k6 =
      skiplist.getElementsInRange("abc", "xyz");
  ASSERT_EQ(k6.size(), 4);
  ASSERT_EQ(k6[0], "key0");
  ASSERT_EQ(k6[3], "key5");

  const std::vector<std::string>& k7 =
      skiplist.getElementsInRange("abc", "aed");
  ASSERT_EQ(k7.size(), 0);

  const std::vector<std::string>& k8 =
      skiplist.getElementsInRange("wxy", "xyz");
  ASSERT_EQ(k8.size(), 0);

  const std::vector<std::string>& k9 =
      skiplist.getElementsInRange("key5", "key0");
  ASSERT_EQ(k9.size(), 0);
}

TEST(SkiplistTest, ArrayAccess) {
  ASSERT_EQ(skiplist[0], "key0");
  ASSERT_EQ(skiplist[1], "key2");
  ASSERT_EQ(skiplist[2], "key4");
  ASSERT_EQ(skiplist[3], "key5");
  ASSERT_EQ(skiplist[skiplist.size() - 1], "key5");

  ASSERT_THROW(skiplist[skiplist.size()], std::out_of_range);
  ASSERT_THROW(skiplist[-1], std::out_of_range);
  ASSERT_THROW(skiplist[INT_MAX], std::out_of_range);
  ASSERT_THROW(skiplist[INT_MIN], std::out_of_range);
}

TEST(SkiplistTest, Iteration) {
  typename Skiplist<std::string>::Iterator it(&skiplist);
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

void scanSkiplist(const Skiplist<std::string>& skiplist) {
  printf("----start scanning skiplist----\n");
  for (auto it = skiplist.begin(); it != skiplist.end(); ++it) {
    printf("%s\n", (*it).c_str());
  }
  printf("----end----\n");

  skiplist.print();
}

}  // namespace skiplist
