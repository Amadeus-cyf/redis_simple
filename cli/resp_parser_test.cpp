#include "cli/resp_parser.h"

#include <gtest/gtest.h>

namespace redis_simple {
namespace cli {
namespace resp_parser {
TEST(RespParserTest, ParseString) {
  std::vector<std::string> reply;
  ASSERT_EQ(resp_parser::Parse("+OK\r\n", reply), 5);
  ASSERT_EQ(reply.back(), "OK");
  ASSERT_EQ(resp_parser::Parse("+123_string\r\n", reply), 13);
  ASSERT_EQ(reply.back(), "123_string");
  ASSERT_EQ(resp_parser::Parse("+\r\n", reply), 3);
  ASSERT_EQ(reply.back(), "");
  ASSERT_EQ(resp_parser::Parse("+OK", reply), -1);
  ASSERT_EQ(resp_parser::Parse("+hello\n\r\n", reply), 9);
  ASSERT_EQ(reply.back(), "hello\n");
}

TEST(RespParserTest, ParseBulkString) {
  std::vector<std::string> reply;
  ASSERT_EQ(resp_parser::Parse("$5\r\nhello\r\n", reply), 11);
  ASSERT_EQ(reply.back(), "hello");
  ASSERT_EQ(resp_parser::Parse("$7\r\nhello\r\n\r\n", reply), 13);
  ASSERT_EQ(reply.back(), "hello\r\n");
  ASSERT_EQ(resp_parser::Parse("$5\r\n", reply), -1);
  ASSERT_EQ(resp_parser::Parse("$5hello\r\n", reply), -1);
  ASSERT_EQ(resp_parser::Parse("$5\r\nhello", reply), -1);
  ASSERT_EQ(resp_parser::Parse("$6\r\nhello\n\r\n", reply), 12);
  ASSERT_EQ(reply.back(), "hello\n");
  ASSERT_EQ(resp_parser::Parse("$15\r\nhello_hello_hel\r\n\r\n\r\n", reply),
            22);
  ASSERT_EQ(reply.back(), "hello_hello_hel");
}

TEST(RespParserTest, ParseInt64) {
  std::vector<std::string> reply;
  ASSERT_EQ(resp_parser::Parse(":123456\r\n", reply), 9);
  ASSERT_EQ(reply.back(), "123456");
  ASSERT_EQ(resp_parser::Parse(":-123456\r\n", reply), 10);
  ASSERT_EQ(reply.back(), "-123456");
  ASSERT_EQ(resp_parser::Parse(":--123456\r\n", reply), -1);
  ASSERT_EQ(resp_parser::Parse(":123456", reply), -1);
}

TEST(ReplyParserTest, ParseArray) {
  // base
  std::vector<std::string> reply1;
  const std::string& s1 =
      "*5\r\n+key1\r\n$5\r\nhello\r\n$7\r\nhello\r\n\r\n:123456\r\n:-12345\r\n";
  ASSERT_EQ(resp_parser::Parse(s1, reply1), s1.size());
  ASSERT_EQ(reply1, std::vector<std::string>({"key1", "hello", "hello\r\n",
                                              "123456", "-12345", "\n"}));
  // empty array
  std::vector<std::string> reply2;
  const std::string& s2 = "*0\r\n";
  ASSERT_EQ(resp_parser::Parse(s2, reply2), 4);
  ASSERT_EQ(reply2, std::vector<std::string>({"\n"}));
  // nested array
  std::vector<std::string> reply3;
  const std::string& s3 =
      "*3\r\n*5\r\n+key1\r\n$5\r\nhello\r\n$7\r\nhello\r\n\r\n:123456\r\n:-"
      "12345\r\n*0\r\n*3\r\n+key2\r\n$7\r\nhello\r\n\r\n:123456\r\n";
  ASSERT_EQ(resp_parser::Parse(s3, reply3), s3.size());
  ASSERT_EQ(reply3, std::vector<std::string>(
                        {"key1", "hello", "hello\r\n", "123456", "-12345", "\n",
                         "\n", "key2", "hello\r\n", "123456", "\n", "\n"}));
}
}  // namespace resp_parser
}  // namespace cli
}  // namespace redis_simple
