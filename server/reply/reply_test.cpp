#include "server/reply/reply.h"

#include <gtest/gtest.h>

namespace redis_simple {
namespace reply {
TEST(ReplyTest, FromSimpleString) {
  ASSERT_EQ(FromString("OK"), "+OK\r\n");
  ASSERT_EQ(FromString(""), "+\r\n");
}

TEST(ReplyTest, FromBulkString) {
  ASSERT_EQ(FromBulkString("test bulk string"), "$16\r\ntest bulk string\r\n");
  ASSERT_EQ(FromBulkString(""), "$0\r\n\r\n");
}

TEST(ReplyTest, From64BitsInt) {
  ASSERT_EQ(FromInt64(1234567), ":1234567\r\n");
}

TEST(ReplyTest, FromArray) {
  ASSERT_EQ(
      FromArray({":123\r\n", "+hello world\r\n", "$13\r\nhello world\r\n"}),
      "*3\r\n:123\r\n+hello world\r\n$13\r\nhello world\r\n");
  ASSERT_THROW(FromArray({":123\r\n", "\r", "$13\r\nhello world"}),
               std::invalid_argument);
  ASSERT_THROW(
      FromArray({":123\r\n", "+hello world\r\n", "$13\r\nhello world"}),
      std::invalid_argument);
}
}  // namespace reply
}  // namespace redis_simple
