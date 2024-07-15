#include "utils/string_utils.h"

#include <gtest/gtest.h>

namespace redis_simple {
namespace utils {
TEST(StringUtilsTest, Split) {
  const std::vector<std::string>& str0 = Split("string for testing", " ");
  ASSERT_EQ(str0, std::vector<std::string>({"string", "for", "testing"}));

  const std::vector<std::string>& str1 =
      Split("string\ntestfor\nnewlinedelimiter\n", "\n");
  ASSERT_EQ(str1, std::vector<std::string>(
                      {"string", "testfor", "newlinedelimiter"}));

  const std::vector<std::string>& str2 = Split("string for test", ",");
  ASSERT_EQ(str2, std::vector<std::string>({"string for test"}));

  const std::vector<std::string>& str3 = Split("", ",");
  ASSERT_TRUE(str3.empty());

  const std::vector<std::string>& str4 = Split("string for testing", "");
  ASSERT_EQ(str4, std::vector<std::string>({"string for testing"}));

  const std::vector<std::string>& str5 = Split("", "");
  ASSERT_EQ(str5, std::vector<std::string>({""}));
}

TEST(StringUtilsTest, ShiftCStr) {
  char s1[14] = {'t', 'e', 's', 't', '_', 'b', 'u',
                 'f', 'f', 'e', 'r', '_', '1'};
  ShiftCStr(s1, 14, 10);
  ASSERT_EQ(std::memcmp(s1, "r_1", sizeof("r_1")), 0);

  char s2[14] = {'t', 'e', 's', 't', '_', 'b', 'u',
                 'f', 'f', 'e', 'r', '_', '2'};
  ShiftCStr(s2, 14, 14);
  ASSERT_EQ(std::memcmp(s2, "", sizeof("")), 0);

  char s3[14] = {'t', 'e', 's', 't', '_', 'b', 'u',
                 'f', 'f', 'e', 'r', '_', '3'};
  ShiftCStr(s3, 14, 0);
  ASSERT_EQ(std::memcmp(s3, "test_buffer_3", sizeof("test_buffer_3")), 0);

  char s4[14] = {'t', 'e', 's', 't', '_', 'b', 'u',
                 'f', 'f', 'e', 'r', '_', '5'};
  ShiftCStr(s4, 14, -1);
  ASSERT_EQ(std::memcmp(s4, "", sizeof("")), 0);

  char* s5 = nullptr;
  ShiftCStr(s5, 0, 0);
  ASSERT_EQ(s5, nullptr);
}

TEST(StringUtilsTest, ToUppercase) {
  std::string s1("test");
  ToUppercase(s1);
  ASSERT_EQ(s1, "TEST");

  std::string s2("TEST");
  ToUppercase(s2);
  ASSERT_EQ(s2, "TEST");

  std::string s3("test_with_miXTure_LETTers_12343546");
  ToUppercase(s3);
  ASSERT_EQ(s3, "TEST_WITH_MIXTURE_LETTERS_12343546");

  std::string s4;
  ToUppercase(s4);
  ASSERT_EQ(s4, "");

  std::string s5("123456_**&&&||||%%%%");
  ToUppercase(s5);
  ASSERT_EQ(s5, "123456_**&&&||||%%%%");
}
}  // namespace utils
}  // namespace redis_simple
