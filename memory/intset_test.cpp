#include "memory/intset.h"

#include <gtest/gtest.h>

namespace redis_simple {
namespace in_memory {
class IntSetTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { intset = new IntSet(); }
  static void TearDownTestSuite() {
    delete intset;
    intset = nullptr;
  }
  static IntSet* intset;
};

IntSet* IntSetTest::intset = nullptr;

TEST_F(IntSetTest, AddAndUpgradeAndGet) {
  ASSERT_TRUE(intset->Add(1));
  ASSERT_EQ(intset->Size(), 1);
  ASSERT_TRUE(intset->Add(5));
  ASSERT_EQ(intset->Size(), 2);
  ASSERT_TRUE(intset->Add(7));
  ASSERT_EQ(intset->Size(), 3);
  ASSERT_TRUE(intset->Add(3));
  ASSERT_EQ(intset->Size(), 4);
  ASSERT_TRUE(intset->Add(2));
  ASSERT_EQ(intset->Size(), 5);
  ASSERT_TRUE(intset->Add(-1));
  ASSERT_EQ(intset->Size(), 6);
  ASSERT_TRUE(intset->Add(INT16_MIN));
  ASSERT_EQ(intset->Size(), 7);
  ASSERT_TRUE(intset->Add(INT16_MAX));
  ASSERT_EQ(intset->Size(), 8);
  ASSERT_TRUE(intset->Add(INT32_MIN));
  ASSERT_EQ(intset->Size(), 9);
  ASSERT_TRUE(intset->Add(INT32_MAX));
  ASSERT_EQ(intset->Size(), 10);
  ASSERT_TRUE(intset->Add(INT64_MIN));
  ASSERT_EQ(intset->Size(), 11);
  ASSERT_TRUE(intset->Add(INT64_MAX));
  ASSERT_EQ(intset->Size(), 12);
  ASSERT_FALSE(intset->Add(1));
  ASSERT_EQ(intset->Size(), 12);
  ASSERT_FALSE(intset->Add(INT64_MIN));
  ASSERT_EQ(intset->Size(), 12);
  ASSERT_FALSE(intset->Add(INT64_MAX));
  ASSERT_EQ(intset->Size(), 12);
  ASSERT_EQ(intset->Size(), 12);

  ASSERT_EQ(intset->Get(0), INT64_MIN);
  ASSERT_EQ(intset->Get(1), INT32_MIN);
  ASSERT_EQ(intset->Get(2), INT16_MIN);
  ASSERT_EQ(intset->Get(3), -1);
  ASSERT_EQ(intset->Get(4), 1);
  ASSERT_EQ(intset->Get(5), 2);
  ASSERT_EQ(intset->Get(6), 3);
  ASSERT_EQ(intset->Get(7), 5);
  ASSERT_EQ(intset->Get(8), 7);
  ASSERT_EQ(intset->Get(9), INT16_MAX);
  ASSERT_EQ(intset->Get(10), INT32_MAX);
  ASSERT_EQ(intset->Get(11), INT64_MAX);
  ASSERT_THROW(intset->Get(-1), std::out_of_range);
  ASSERT_THROW(intset->Get(12), std::out_of_range);
}

TEST_F(IntSetTest, Find) {
  ASSERT_TRUE(intset->Find(INT64_MIN));
  ASSERT_TRUE(intset->Find(INT64_MAX));
  ASSERT_TRUE(intset->Find(INT16_MIN));
  ASSERT_TRUE(intset->Find(INT16_MAX));
  ASSERT_TRUE(intset->Find(-1));
  ASSERT_TRUE(intset->Find(7));
  ASSERT_FALSE(intset->Find(-7));
  ASSERT_FALSE(intset->Find(0));
  ASSERT_FALSE(intset->Find(INT64_MIN + 1));
  ASSERT_FALSE(intset->Find(INT64_MAX - 1));
  ASSERT_FALSE(intset->Find(INT32_MIN + 1));
  ASSERT_FALSE(intset->Find(INT32_MAX - 1));
}

TEST_F(IntSetTest, Remove) {
  ASSERT_TRUE(intset->Remove(INT16_MAX));
  ASSERT_FALSE(intset->Find(INT16_MAX));
  ASSERT_EQ(intset->Size(), 11);
  ASSERT_FALSE(intset->Remove(INT16_MAX));
  ASSERT_EQ(intset->Size(), 11);

  ASSERT_TRUE(intset->Remove(INT64_MIN));
  ASSERT_FALSE(intset->Find(INT64_MIN));
  ASSERT_EQ(intset->Size(), 10);
  ASSERT_FALSE(intset->Remove(INT64_MIN));
  ASSERT_EQ(intset->Size(), 10);

  ASSERT_TRUE(intset->Remove(INT64_MAX));
  ASSERT_FALSE(intset->Find(INT64_MAX));
  ASSERT_EQ(intset->Size(), 9);
  ASSERT_FALSE(intset->Remove(INT64_MAX));
  ASSERT_EQ(intset->Size(), 9);
}

TEST_F(IntSetTest, Iterator) {
  IntSet::Iterator it(intset);
  it.SeekToFirst();
  ASSERT_EQ(it.Value(), INT32_MIN);
  it.Next();
  ASSERT_EQ(it.Value(), INT16_MIN);
  it.Next();
  ASSERT_EQ(it.Value(), -1);
  it.Next();
  ASSERT_EQ(it.Value(), 1);
  it.Next();
  ASSERT_EQ(it.Value(), 2);
  it.Next();
  ASSERT_EQ(it.Value(), 3);
  it.Next();
  ASSERT_EQ(it.Value(), 5);
  it.Next();
  ASSERT_EQ(it.Value(), 7);
  it.SeekToLast();
  ASSERT_EQ(it.Value(), INT32_MAX);
}
}  // namespace in_memory
}  // namespace redis_simple
