#include "server/commands/command.h"

#include <array>

#include "server/commands/handlers.h"
#include "utils/string_utils.h"

namespace redis_simple::command {
namespace {
constexpr std::array<Command, 22> kCommandTable = {{
    {"GET", strings::HandleGet},
    {"SET", strings::HandleSet},
    {"DEL", key::HandleDel},
    {"EXISTS", key::HandleExists},
    {"TYPE", key::HandleType},
    {"LPUSH", lists::HandleLPush},
    {"RPUSH", lists::HandleRPush},
    {"LPOP", lists::HandleLPop},
    {"RPOP", lists::HandleRPop},
    {"LLEN", lists::HandleLLen},
    {"LRANGE", lists::HandleLRange},
    {"SADD", sets::HandleSAdd},
    {"SCARD", sets::HandleSCard},
    {"SREM", sets::HandleSRem},
    {"SMEMBERS", sets::HandleSMembers},
    {"SISMEMBER", sets::HandleSIsMember},
    {"ZADD", zsets::HandleZAdd},
    {"ZCARD", zsets::HandleZCard},
    {"ZREM", zsets::HandleZRem},
    {"ZRANK", zsets::HandleZRank},
    {"ZRANGE", zsets::HandleZRange},
    {"ZSCORE", zsets::HandleZScore},
}};
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
