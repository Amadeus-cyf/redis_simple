#include "server/commands/command.h"

#include <array>
#include <cctype>
#include <string_view>

#include "server/commands/handlers.h"

namespace redis_simple::command {
namespace {
constexpr std::array kCommandTable = {
    Command{"GET", strings::HandleGet},
    Command{"SET", strings::HandleSet},
    Command{"INCR", strings::HandleIncr},
    Command{"DECR", strings::HandleDecr},
    Command{"APPEND", strings::HandleAppend},
    Command{"MGET", strings::HandleMGet},
    Command{"MSET", strings::HandleMSet},
    Command{"DEL", key::HandleDel},
    Command{"UNLINK", key::HandleUnlink},
    Command{"EXISTS", key::HandleExists},
    Command{"TYPE", key::HandleType},
    Command{"EXPIRE", key::HandleExpire},
    Command{"PEXPIRE", key::HandlePExpire},
    Command{"TTL", key::HandleTtl},
    Command{"PTTL", key::HandlePTtl},
    Command{"PERSIST", key::HandlePersist},
    Command{"RENAME", key::HandleRename},
    Command{"DBSIZE", key::HandleDbSize},
    Command{"FLUSHDB", key::HandleFlushDb},
    Command{"LPUSH", lists::HandleLPush},
    Command{"RPUSH", lists::HandleRPush},
    Command{"LPOP", lists::HandleLPop},
    Command{"RPOP", lists::HandleRPop},
    Command{"LLEN", lists::HandleLLen},
    Command{"LRANGE", lists::HandleLRange},
    Command{"LINDEX", lists::HandleLIndex},
    Command{"LSET", lists::HandleLSet},
    Command{"LREM", lists::HandleLRem},
    Command{"LTRIM", lists::HandleLTrim},
    Command{"SADD", sets::HandleSAdd},
    Command{"SCARD", sets::HandleSCard},
    Command{"SREM", sets::HandleSRem},
    Command{"SMEMBERS", sets::HandleSMembers},
    Command{"SISMEMBER", sets::HandleSIsMember},
    Command{"SINTER", sets::HandleSInter},
    Command{"SUNION", sets::HandleSUnion},
    Command{"SDIFF", sets::HandleSDiff},
    Command{"ZADD", zsets::HandleZAdd},
    Command{"ZCARD", zsets::HandleZCard},
    Command{"ZREM", zsets::HandleZRem},
    Command{"ZRANK", zsets::HandleZRank},
    Command{"ZRANGE", zsets::HandleZRange},
    Command{"ZREVRANGE", zsets::HandleZRevRange},
    Command{"ZRANGEBYSCORE", zsets::HandleZRangeByScore},
    Command{"ZCOUNT", zsets::HandleZCount},
    Command{"ZSCORE", zsets::HandleZScore},
    Command{"HSET", hashes::HandleHSet},
    Command{"HGET", hashes::HandleHGet},
    Command{"HDEL", hashes::HandleHDel},
    Command{"HLEN", hashes::HandleHLen},
    Command{"HEXISTS", hashes::HandleHExists},
    Command{"HGETALL", hashes::HandleHGetAll},
    Command{"HMGET", hashes::HandleHMGet},
    Command{"HKEYS", hashes::HandleHKeys},
    Command{"HVALS", hashes::HandleHVals},
    Command{"HINCRBY", hashes::HandleHIncrBy},
};

bool CommandNameEquals(std::string_view input, std::string_view registered) {
  if (input.size() != registered.size()) {
    return false;
  }
  for (size_t index = 0; index < input.size(); ++index) {
    if (std::toupper(static_cast<unsigned char>(input[index])) !=
        registered[index]) {
      return false;
    }
  }
  return true;
}
}  // namespace

const Command* Find(std::string_view name) {
  for (const Command& command : kCommandTable) {
    if (CommandNameEquals(name, command.name)) {
      return &command;
    }
  }
  return nullptr;
}
}  // namespace redis_simple::command
