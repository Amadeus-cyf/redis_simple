#include "server/commands/command.h"

#include <gtest/gtest.h>

namespace redis_simple::command {
TEST(CommandRegistryTest, FindsCommandsCaseInsensitively) {
  const auto* get = Find("get");
  ASSERT_NE(get, nullptr);
  EXPECT_STREQ(get->name, "GET");
  EXPECT_NE(get->callback, nullptr);

  const auto* zrange = Find("ZRANGE");
  ASSERT_NE(zrange, nullptr);
  EXPECT_STREQ(zrange->name, "ZRANGE");
  EXPECT_NE(zrange->callback, nullptr);
}

TEST(CommandRegistryTest, ReturnsNullForUnknownCommand) {
  EXPECT_EQ(Find("UNKNOWN"), nullptr);
  EXPECT_EQ(Find(""), nullptr);
}
}  // namespace redis_simple::command
