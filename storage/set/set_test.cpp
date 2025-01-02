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

TEST_F(SetTest, AddAndList) {
  // Intset encoding
  ASSERT_TRUE(set->Add("1"));
  ASSERT_EQ(set->Size(), 1);
  ASSERT_TRUE(set->HasMember("1"));
  ASSERT_FALSE(set->Add("1"));

  ASSERT_TRUE(set->Add("65535"));
  ASSERT_EQ(set->Size(), 2);
  ASSERT_TRUE(set->HasMember("65535"));
  ASSERT_FALSE(set->Add("65535"));

  ASSERT_TRUE(set->Add("-65536"));
  ASSERT_EQ(set->Size(), 3);
  ASSERT_TRUE(set->HasMember("-65536"));
  ASSERT_FALSE(set->Add("-65536"));

  ASSERT_TRUE(set->Add("4294967295"));
  ASSERT_EQ(set->Size(), 4);
  ASSERT_TRUE(set->HasMember("4294967295"));
  ASSERT_FALSE(set->Add("4294967295"));

  ASSERT_TRUE(set->Add("-4294967296"));
  ASSERT_EQ(set->Size(), 5);
  ASSERT_TRUE(set->HasMember("-4294967296"));
  ASSERT_FALSE(set->Add("-4294967296"));

  ASSERT_TRUE(set->Add("9223372036854775807"));
  ASSERT_EQ(set->Size(), 6);
  ASSERT_TRUE(set->HasMember("9223372036854775807"));
  ASSERT_FALSE(set->Add("9223372036854775807"));

  ASSERT_TRUE(set->Add("-9223372036854775808"));
  ASSERT_EQ(set->Size(), 7);
  ASSERT_TRUE(set->HasMember("-9223372036854775808"));
  ASSERT_FALSE(set->Add("-9223372036854775808"));

  const std::vector<std::string>& members_intset = set->ListAllMembers();
  ASSERT_EQ(members_intset,
            std::vector<std::string>({"-9223372036854775808", "-4294967296",
                                      "-65536", "1", "65535", "4294967295",
                                      "9223372036854775807"}));

  // Convert to listpack
  ASSERT_TRUE(set->Add("test_str_0"));
  ASSERT_EQ(set->Size(), 8);
  ASSERT_TRUE(set->HasMember("test_str_0"));
  ASSERT_FALSE(set->Add("test_str_0"));

  ASSERT_TRUE(set->Add("test_str_1"));
  ASSERT_EQ(set->Size(), 9);
  ASSERT_TRUE(set->HasMember("test_str_1"));
  ASSERT_FALSE(set->Add("test_str_1"));
  ASSERT_TRUE(set->HasMember("-9223372036854775808"));
  ASSERT_TRUE(set->HasMember("4294967295"));

  const std::vector<std::string>& members_dict = set->ListAllMembers();
  ASSERT_EQ(members_dict.size(), 9);
  for (const std::string& member : members_dict) {
    printf("member: %s\n", member.c_str());
  }
}

TEST_F(SetTest, Remove) {
  ASSERT_TRUE(set->Remove("test_str_0"));
  ASSERT_FALSE(set->HasMember("test_str_0"));
  ASSERT_EQ(set->Size(), 8);

  ASSERT_TRUE(set->Remove("-9223372036854775808"));
  ASSERT_FALSE(set->HasMember("-9223372036854775808"));
  ASSERT_EQ(set->Size(), 7);

  ASSERT_TRUE(set->Remove("9223372036854775807"));
  ASSERT_FALSE(set->HasMember("9223372036854775807"));
  ASSERT_EQ(set->Size(), 6);

  ASSERT_TRUE(set->Remove("4294967295"));
  ASSERT_FALSE(set->HasMember("4294967295"));
  ASSERT_EQ(set->Size(), 5);

  ASSERT_TRUE(set->Remove("65535"));
  ASSERT_FALSE(set->HasMember("65535"));
  ASSERT_EQ(set->Size(), 4);

  ASSERT_TRUE(set->Remove("1"));
  ASSERT_FALSE(set->HasMember("1"));
  ASSERT_EQ(set->Size(), 3);

  ASSERT_FALSE(set->Remove("0"));
  ASSERT_EQ(set->Size(), 3);
  ASSERT_FALSE(set->Remove("test_key_not_exist"));
  ASSERT_EQ(set->Size(), 3);
}
}  // namespace set
}  // namespace redis_simple
