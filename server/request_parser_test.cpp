#include "server/request_parser.h"

#include <gtest/gtest.h>

#include <string_view>

namespace redis_simple::request_parser {
TEST(RequestParserTest, ParsesRespBulkStringArrayWithoutCopying) {
  constexpr std::string_view kRequest =
      "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$11\r\nhello world\r\n";
  std::string_view command;
  command::CommandArgs args;

  const auto result = Parse(kRequest, &command, &args);

  EXPECT_EQ(result.status, ParseStatus::kComplete);
  EXPECT_EQ(result.consumed, kRequest.size());
  EXPECT_EQ(command, "SET");
  ASSERT_EQ(args.size(), 2);
  EXPECT_EQ(args[0], "key");
  EXPECT_EQ(args[1], "hello world");
  EXPECT_GE(command.data(), kRequest.data());
  EXPECT_LT(command.data(), kRequest.data() + kRequest.size());
}

TEST(RequestParserTest, DistinguishesIncompleteAndInvalidResp) {
  std::string_view command;
  command::CommandArgs args;

  EXPECT_EQ(Parse("*2\r\n$3\r\nGET\r\n$3\r\nke", &command, &args).status,
            ParseStatus::kIncomplete);
  EXPECT_TRUE(command.empty());
  EXPECT_TRUE(args.empty());
  EXPECT_EQ(Parse("*1\r\n+GET\r\n", &command, &args).status,
            ParseStatus::kInvalid);
  EXPECT_EQ(Parse("*0\r\n", &command, &args).status, ParseStatus::kInvalid);
}

TEST(RequestParserTest, BoundsReservationForIncompleteResp) {
  constexpr std::string_view kRequest = "*1048576\r\n";
  std::string_view command;
  command::CommandArgs args;

  EXPECT_EQ(Parse(kRequest, &command, &args).status, ParseStatus::kIncomplete);
  EXPECT_LE(args.capacity(), kRequest.size());
}

TEST(RequestParserTest, PreservesInlineProtocolCompatibility) {
  std::string_view command;
  command::CommandArgs args;

  const auto result = Parse("  GET   key  \r\nremaining", &command, &args);

  EXPECT_EQ(result.status, ParseStatus::kComplete);
  EXPECT_EQ(result.consumed, 15);
  EXPECT_EQ(command, "GET");
  ASSERT_EQ(args.size(), 1);
  EXPECT_EQ(args[0], "key");
}
}  // namespace redis_simple::request_parser
