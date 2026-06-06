#include "server/reply_utils/reply_utils.h"

#include <gtest/gtest.h>

#include <vector>

namespace redis_simple {
namespace reply_utils {
TEST(TestReplyUtils, EncodeList) {
  const auto l1 =
      std::vector<std::string>{"element_0", "element_1", "element_2"};
  auto to_string_1 = [](const std::string& s) { return s; };
  const auto opt1 = reply_utils::EncodeList<std::string, to_string_1>(l1);
  ASSERT_EQ(opt1.value_or(""),
            "*3\r\n$9\r\nelement_0\r\n$9\r\nelement_1\r\n$9\r\nelement_2\r\n");

  const auto l2 = std::vector<int>{123, 1234, 12345};
  auto to_string_2 = [](const int& i) { return std::to_string(i); };
  const auto opt2 = reply_utils::EncodeList<int, to_string_2>(l2);
  ASSERT_EQ(opt2.value_or(""), "*3\r\n:123\r\n:1234\r\n:12345\r\n");

  const auto l3 = std::vector<std::string>{};
  const auto opt3 = reply_utils::EncodeList<std::string, to_string_1>(l3);
  ASSERT_EQ(opt3.value_or(""), "*0\r\n");
}
}  // namespace reply_utils
}  // namespace redis_simple
