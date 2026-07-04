#include "server/commands/command.h"

#include <array>

#include "server/commands/handlers.h"
#include "utils/string_utils.h"

namespace redis_simple::command {
namespace {
constexpr std::array kCommandTable = {
    Command{"GET", strings::HandleGet},
    Command{"SET", strings::HandleSet},
    Command{"DEL", key::HandleDel},
    Command{"EXISTS", key::HandleExists},
    Command{"TYPE", key::HandleType},
    Command{"LPUSH", lists::HandleLPush},
    Command{"RPUSH", lists::HandleRPush},
    Command{"LPOP", lists::HandleLPop},
    Command{"RPOP", lists::HandleRPop},
    Command{"LLEN", lists::HandleLLen},
    Command{"LRANGE", lists::HandleLRange},
    Command{"SADD", sets::HandleSAdd},
    Command{"SCARD", sets::HandleSCard},
    Command{"SREM", sets::HandleSRem},
    Command{"SMEMBERS", sets::HandleSMembers},
    Command{"SISMEMBER", sets::HandleSIsMember},
    Command{"ZADD", zsets::HandleZAdd},
    Command{"ZCARD", zsets::HandleZCard},
    Command{"ZREM", zsets::HandleZRem},
    Command{"ZRANK", zsets::HandleZRank},
    Command{"ZRANGE", zsets::HandleZRange},
    Command{"ZSCORE", zsets::HandleZScore},
    Command{"HSET", hashes::HandleHSet},
    Command{"HGET", hashes::HandleHGet},
    Command{"HDEL", hashes::HandleHDel},
    Command{"HLEN", hashes::HandleHLen},
    Command{"HEXISTS", hashes::HandleHExists},
    Command{"HGETALL", hashes::HandleHGetAll},
};
}  // namespace

const Command* Find(const std::string& name) {
  auto upper_name = name;
  utils::ToUppercase(upper_name);
  for (const Command& command : kCommandTable) {
    if (upper_name == command.name) {
      return &command;
    }
  }
  return nullptr;
}
}  // namespace redis_simple::command
