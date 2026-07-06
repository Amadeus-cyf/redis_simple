#include "server/reply/reply.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace redis_simple::reply {
TEST(ReplyTest, FromSimpleString) {
  ASSERT_EQ(FromString("OK"), "+OK\r\n");
  ASSERT_EQ(FromString(""), "+\r\n");
}

TEST(ReplyTest, FromBulkString) {
  ASSERT_EQ(FromBulkString("test bulk string"), "$16\r\ntest bulk string\r\n");
  ASSERT_EQ(FromBulkString(""), "$0\r\n\r\n");
}

TEST(ReplyTest, FromBulkStringArray) {
  ASSERT_EQ(FromBulkStringArray({"one", "two"}),
            "*2\r\n$3\r\none\r\n$3\r\ntwo\r\n");
  ASSERT_EQ(FromBulkStringArray({}), "*0\r\n");
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

TEST(ReplyTest, FromError) {
  ASSERT_EQ(FromError("ERR syntax error"), "-ERR syntax error\r\n");
  ASSERT_EQ(WrongNumberOfArguments(), "-ERR wrong number of arguments\r\n");
}
}  // namespace redis_simple::reply
