#include "server/commands/command.h"

#include <array>

#include "utils/string_utils.h"

namespace redis_simple {
class Client;
}  // namespace redis_simple

namespace redis_simple::command::key {
void HandleDel(Client* client);
}  // namespace redis_simple::command::key

namespace redis_simple::command::strings {
void HandleGet(Client* client);
void HandleSet(Client* client);
}  // namespace redis_simple::command::strings

namespace redis_simple::command::lists {
void HandleLPush(Client* client);
void HandleRPush(Client* client);
void HandleLPop(Client* client);
void HandleRPop(Client* client);
void HandleLLen(Client* client);
void HandleLRange(Client* client);
}  // namespace redis_simple::command::lists

namespace redis_simple::command::sets {
void HandleSAdd(Client* client);
void HandleSCard(Client* client);
void HandleSIsMember(Client* client);
void HandleSMembers(Client* client);
void HandleSRem(Client* client);
}  // namespace redis_simple::command::sets

namespace redis_simple::command::zsets {
void HandleZAdd(Client* client);
void HandleZCard(Client* client);
void HandleZRange(Client* client);
void HandleZRank(Client* client);
void HandleZRem(Client* client);
void HandleZScore(Client* client);
}  // namespace redis_simple::command::zsets

namespace redis_simple::command {
namespace {
constexpr std::array<Command, 20> kCommandTable = {{
    {"GET", strings::HandleGet},
    {"SET", strings::HandleSet},
    {"DEL", key::HandleDel},
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
