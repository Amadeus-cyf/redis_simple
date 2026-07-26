#include "server/commands/command.h"

#include <gtest/gtest.h>

#include <array>

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

  const auto* hset = Find("hset");
  ASSERT_NE(hset, nullptr);
  EXPECT_STREQ(hset->name, "HSET");
  EXPECT_NE(hset->callback, nullptr);
}

TEST(CommandRegistryTest, FindsExpandedRedisCommandSet) {
  const std::array names = {
      "EXPIRE", "PEXPIRE", "TTL",     "PTTL",      "PERSIST",       "RENAME",
      "DBSIZE", "FLUSHDB", "INCR",    "DECR",      "APPEND",        "MGET",
      "MSET",   "LINDEX",  "LSET",    "LREM",      "LTRIM",         "SINTER",
      "SUNION", "SDIFF",   "ZCOUNT",  "ZREVRANGE", "ZRANGEBYSCORE", "HMGET",
      "HKEYS",  "HVALS",   "HINCRBY", "HELLO",     "PING",          "ECHO",
      "QUIT",   "SCAN"};
  for (const auto* name : names) {
    const auto* command = Find(name);
    ASSERT_NE(command, nullptr) << name;
    EXPECT_NE(command->callback, nullptr);
  }
}

TEST(CommandRegistryTest, ReturnsNullForUnknownCommand) {
  EXPECT_EQ(Find("UNKNOWN"), nullptr);
  EXPECT_EQ(Find(""), nullptr);
}
}  // namespace redis_simple::command
