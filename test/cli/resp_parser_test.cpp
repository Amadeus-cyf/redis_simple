#include "cli/resp_parser.h"

#include <gtest/gtest.h>

namespace redis_simple {
namespace cli {
namespace resp_parser {
TEST(RespParserTest, ParseString) {
  std::string reply;
  ASSERT_EQ(resp_parser::parse("+OK\r\n", reply), 5);
  ASSERT_EQ(reply, "OK");
  ASSERT_EQ(resp_parser::parse("+123_string\r\n", reply), 13);
  ASSERT_EQ(reply, "123_string");
  ASSERT_EQ(resp_parser::parse("+\r\n", reply), 3);
  ASSERT_EQ(reply, "");
  ASSERT_EQ(resp_parser::parse("+OK", reply), -1);
  ASSERT_EQ(resp_parser::parse("+hello\n\r\n", reply), 9);
  ASSERT_EQ(reply, "hello\n");
}

TEST(RespParserTest, ParseBulkString) {
  std::string reply;
  ASSERT_EQ(resp_parser::parse("$5\r\nhello\r\n", reply), 11);
  ASSERT_EQ(reply, "hello");
  ASSERT_EQ(resp_parser::parse("$7\r\nhello\r\n\r\n", reply), 13);
  ASSERT_EQ(reply, "hello\r\n");
  ASSERT_EQ(resp_parser::parse("$5\r\n", reply), -1);
  ASSERT_EQ(resp_parser::parse("$5hello\r\n", reply), -1);
  ASSERT_EQ(resp_parser::parse("$5\r\nhello", reply), -1);
  ASSERT_EQ(resp_parser::parse("$6\r\nhello\n\r\n", reply), 12);
  ASSERT_EQ(reply, "hello\n");
  ASSERT_EQ(resp_parser::parse("$15\r\nhello_hello_hel\r\n\r\n\r\n", reply),
            22);
  ASSERT_EQ(reply, "hello_hello_hel");
}

TEST(RespParserTest, ParseInt64) {
  std::string reply;
  ASSERT_EQ(resp_parser::parse(":123456\r\n", reply), 9);
  ASSERT_EQ(reply, "123456");
  ASSERT_EQ(resp_parser::parse(":-123456\r\n", reply), 10);
  ASSERT_EQ(reply, "-123456");
  ASSERT_EQ(resp_parser::parse(":--123456\r\n", reply), -1);
  ASSERT_EQ(resp_parser::parse(":123456", reply), -1);
}
}  // namespace resp_parser
}  // namespace cli
}  // namespace redis_simple
