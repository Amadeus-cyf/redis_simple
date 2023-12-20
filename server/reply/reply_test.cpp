#include "server/reply/reply.h"

#include <gtest/gtest.h>

namespace redis_simple {
namespace reply {
TEST(ReplyTest, FromSimpleString) {
  const std::string& reply = FromString("OK");
  ASSERT_EQ(reply, "+OK\r\n");
}

TEST(ReplyTest, FromBulkString) {
  const std::string& reply = FromBulkString("test bulk string");
  ASSERT_EQ(reply, "$16\r\ntest bulk string\r\n");
}

TEST(ReplyTest, From64BitsInt) {
  const std::string& reply = FromInt64(1234567);
  ASSERT_EQ(reply, ":1234567\r\n");
}
}  // namespace reply
}  // namespace redis_simple
