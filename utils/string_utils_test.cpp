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
                 'f', 'f', 'e', 'r', '_', '4'};
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

TEST(StringUtilsTest, ToInt64) {
  std::string s1("100234567");
  int64_t v1;
  ASSERT_TRUE(ToInt64(s1, &v1));
  ASSERT_EQ(v1, 100234567);

  std::string s2("-100234567");
  int64_t v2;
  ASSERT_TRUE(ToInt64(s2, &v2));
  ASSERT_EQ(v2, -100234567);

  std::string s3("+100234567");
  int64_t v3;
  ASSERT_TRUE(ToInt64(s3, &v3));
  ASSERT_EQ(v3, 100234567);

  std::string s4("0");
  int64_t v4;
  ASSERT_TRUE(ToInt64(s4, &v4));
  ASSERT_EQ(v4, 0);

  std::string s5(" -1234545   ");
  ASSERT_FALSE(ToInt64(s5, nullptr));

  std::string s6(" -1234 abcde");
  ASSERT_FALSE(ToInt64(s6, nullptr));

  std::string s7("-12345-123");
  ASSERT_FALSE(ToInt64(s7, nullptr));

  std::string s8("-12345 56778");
  ASSERT_FALSE(ToInt64(s8, nullptr));

  std::string s9("0000000");
  ASSERT_FALSE(ToInt64(s9, nullptr));

  std::string s10("+0");
  int64_t v10;
  ASSERT_TRUE(ToInt64(s10, &v10));
  ASSERT_EQ(v10, 0);

  std::string s11("-0");
  int64_t v11;
  ASSERT_TRUE(ToInt64(s11, &v11));
  ASSERT_EQ(v11, 0);

  std::string s12("0123456");
  ASSERT_FALSE(ToInt64(s12, nullptr));

  std::string s13("1002345670000000");
  int64_t v13;
  ASSERT_TRUE(ToInt64(s13, &v13));
  ASSERT_EQ(v13, 1002345670000000);

  std::string s14("9223372036854775807");
  int64_t v14;
  ASSERT_TRUE(ToInt64(s14, &v14));
  ASSERT_EQ(v14, INT64_MAX);

  std::string s15("-9223372036854775808");
  int64_t v15;
  ASSERT_TRUE(ToInt64(s15, &v15));
  ASSERT_EQ(v15, INT64_MIN);

  std::string s16("9223372036854775808");
  ASSERT_FALSE(ToInt64(s16, nullptr));

  std::string s17("-9223372036854775809");
  ASSERT_FALSE(ToInt64(s17, nullptr));

  std::string s18("-922337203685477580812312");
  ASSERT_FALSE(ToInt64(s18, nullptr));

  std::string s19("");
  ASSERT_FALSE(ToInt64(s19, nullptr));
}

TEST(StringUtilsTest, LL2String) {
  char dst1[21];
  std::string s1("1234567");
  ASSERT_EQ(LL2String(dst1, 21, 1234567), 7);
  ASSERT_TRUE(std::equal(dst1, dst1 + 7, s1.c_str()));

  char dst2[21];
  std::string s2("-1234567");
  ASSERT_EQ(LL2String(dst2, 21, -1234567), 8);
  ASSERT_TRUE(std::equal(dst2, dst2 + 8, s2.c_str()));

  char dst3[21];
  std::string s3("9223372036854775807");
  ASSERT_EQ(LL2String(dst3, 21, INT64_MAX), 19);
  ASSERT_TRUE(std::equal(dst3, dst3 + 19, s3.c_str()));

  char dst4[21];
  std::string s4("-9223372036854775808");
  ASSERT_EQ(LL2String(dst4, 21, INT64_MIN), 20);
  ASSERT_TRUE(std::equal(dst4, dst4 + 20, s4.c_str()));

  char dst5[21];
  std::string s5("0");
  ASSERT_EQ(LL2String(dst5, 21, 0), 1);
  ASSERT_TRUE(std::equal(dst5, dst5 + 1, s5.c_str()));

  char dst6[1];
  std::string s6("1000");
  ASSERT_EQ(LL2String(dst6, 1, 0), 0);
  ASSERT_EQ(dst6[0], '\0');

  char dst7[7];
  std::string s7("10000000");
  ASSERT_EQ(LL2String(dst7, 7, 10000000), 0);
  ASSERT_EQ(dst7[0], '\0');

  char dst8[7];
  std::string s8("-10000000");
  ASSERT_EQ(LL2String(dst8, 7, -10000000), 0);
  ASSERT_EQ(dst8[0], '-');
  ASSERT_EQ(dst8[1], '\0');
}

TEST(StringUtilsTest, Ull2String) {
  char dst1[21];
  std::string s1("1234567");
  ASSERT_EQ(Ull2String(dst1, 21, 1234567), 7);
  ASSERT_TRUE(std::equal(dst1, dst1 + 7, s1.c_str()));

  char dst2[21];
  std::string s2("9223372036854775807");
  ASSERT_EQ(Ull2String(dst2, 21, INT64_MAX), 19);
  ASSERT_TRUE(std::equal(dst2, dst2 + 19, s2.c_str()));

  char dst3[21];
  std::string s3("18446744073709551615");
  ASSERT_EQ(Ull2String(dst3, 21, UINT64_MAX), 20);
  ASSERT_TRUE(std::equal(dst3, dst3 + 20, s3.c_str()));

  char dst4[21];
  std::string s4("0");
  ASSERT_EQ(Ull2String(dst4, 21, 0), 1);
  ASSERT_TRUE(std::equal(dst4, dst4 + 1, s4.c_str()));

  char dst5[7];
  std::string s5("10000000");
  ASSERT_EQ(Ull2String(dst5, 7, 10000000), 0);
  ASSERT_EQ(dst5[0], '\0');
}
}  // namespace utils
}  // namespace redis_simple
