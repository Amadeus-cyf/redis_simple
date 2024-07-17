#include "server/reply_utils/reply_utils.h"

#include <gtest/gtest.h>

namespace redis_simple {
namespace reply_utils {
TEST(TestReplyUtils, EncodeList) {
  const std::vector<std::string>& l1 = {"element_0", "element_1", "element_2"};
  auto to_string_1 = [](const std::string& s) { return s; };
  const std::optional<std::string>& opt1 =
      reply_utils::EncodeList<std::string, to_string_1>(l1);
  ASSERT_EQ(opt1.value_or(""),
            "*3\r\n$9\r\nelement_0\r\n$9\r\nelement_1\r\n$9\r\nelement_2\r\n");

  const std::vector<int>& l2 = {123, 1234, 12345};
  auto to_string_2 = [](const int& i) { return std::to_string(i); };
  const std::optional<std::string>& opt2 =
      reply_utils::EncodeList<int, to_string_2>(l2);
  ASSERT_EQ(opt2.value_or(""), "*3\r\n:123\r\n:1234\r\n:12345\r\n");

  const std::vector<std::string>& l3 = {};
  const std::optional<std::string>& opt3 =
      reply_utils::EncodeList<std::string, to_string_1>(l3);
  ASSERT_EQ(opt3.value_or(""), "*0\r\n");
}
}  // namespace reply_utils
}  // namespace redis_simple
