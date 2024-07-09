#include "set.h"

#include <gtest/gtest.h>

namespace redis_simple {
namespace set {
class SetTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { set = Set::Init(); }
  static void TearDownTestSuit() {
    delete set;
    set = nullptr;
  }
  static Set* set;
};

Set* SetTest::set = nullptr;

TEST_F(SetTest, Add) {
  /* intset encoding */
  ASSERT_TRUE(set->Add("1"));
  ASSERT_EQ(set->Size(), 1);
  ASSERT_TRUE(set->Contains("1"));
  ASSERT_FALSE(set->Add("1"));

  ASSERT_TRUE(set->Add("65535"));
  ASSERT_EQ(set->Size(), 2);
  ASSERT_TRUE(set->Contains("65535"));
  ASSERT_FALSE(set->Add("65535"));

  ASSERT_TRUE(set->Add("-65536"));
  ASSERT_EQ(set->Size(), 3);
  ASSERT_TRUE(set->Contains("-65536"));
  ASSERT_FALSE(set->Add("-65536"));

  ASSERT_TRUE(set->Add("4294967295"));
  ASSERT_EQ(set->Size(), 4);
  ASSERT_TRUE(set->Contains("4294967295"));
  ASSERT_FALSE(set->Add("4294967295"));

  ASSERT_TRUE(set->Add("-4294967296"));
  ASSERT_EQ(set->Size(), 5);
  ASSERT_TRUE(set->Contains("-4294967296"));
  ASSERT_FALSE(set->Add("-4294967296"));

  ASSERT_TRUE(set->Add("9223372036854775807"));
  ASSERT_EQ(set->Size(), 6);
  ASSERT_TRUE(set->Contains("9223372036854775807"));
  ASSERT_FALSE(set->Add("9223372036854775807"));

  ASSERT_TRUE(set->Add("-9223372036854775808"));
  ASSERT_EQ(set->Size(), 7);
  ASSERT_TRUE(set->Contains("-9223372036854775808"));
  ASSERT_FALSE(set->Add("-9223372036854775808"));

  /* convert to dict encoding */
  ASSERT_TRUE(set->Add("test_str_0"));
  ASSERT_EQ(set->Size(), 8);
  ASSERT_TRUE(set->Contains("test_str_0"));
  ASSERT_FALSE(set->Add("test_str_0"));

  ASSERT_TRUE(set->Add("test_str_1"));
  ASSERT_EQ(set->Size(), 9);
  ASSERT_TRUE(set->Contains("test_str_1"));
  ASSERT_FALSE(set->Add("test_str_1"));
}
}  // namespace set
}  // namespace redis_simple
