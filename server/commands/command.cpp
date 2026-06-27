#include "server/commands/command.h"

#include "server/commands/t_set/sadd.h"
#include "server/commands/t_set/scard.h"
#include "server/commands/t_set/sismember.h"
#include "server/commands/t_set/smembers.h"
#include "server/commands/t_set/srem.h"
#include "server/commands/t_string/delete.h"
#include "server/commands/t_string/get.h"
#include "server/commands/t_string/set.h"
#include "server/commands/t_zset/zadd.h"
#include "server/commands/t_zset/zcard.h"
#include "server/commands/t_zset/zrange.h"
#include "server/commands/t_zset/zrank.h"
#include "server/commands/t_zset/zrem.h"
#include "server/commands/t_zset/zscore.h"
#include "utils/string_utils.h"

namespace redis_simple::command {
namespace {
const std::unordered_map<std::string, Command> kCommandTable = {
    {"GET", {"GET", t_string::ExecuteGet}},
    {"SET", {"SET", t_string::ExecuteSet}},
    {"DEL", {"DEL", t_string::ExecuteDelete}},
    {"SADD", {"SADD", t_set::ExecuteSAdd}},
    {"SCARD", {"SCARD", t_set::ExecuteSCard}},
    {"SREM", {"SREM", t_set::ExecuteSRem}},
    {"SMEMBERS", {"SMEMBERS", t_set::ExecuteSMembers}},
    {"SISMEMBER", {"SISMEMBER", t_set::ExecuteSIsMember}},
    {"ZADD", {"ZADD", t_zset::ExecuteZAdd}},
    {"ZCARD", {"ZCARD", t_zset::ExecuteZCard}},
    {"ZREM", {"ZREM", t_zset::ExecuteZRem}},
    {"ZRANK", {"ZRANK", t_zset::ExecuteZRank}},
    {"ZRANGE", {"ZRANGE", t_zset::ExecuteZRange}},
    {"ZSCORE", {"ZSCORE", t_zset::ExecuteZScore}},
};
}  // namespace

const Command* Find(const std::string& name) {
  auto upper_name = name;
  utils::ToUppercase(upper_name);
  try {
    return &kCommandTable.at(upper_name);
  } catch (const std::out_of_range&) {
    return nullptr;
  }
}
}  // namespace redis_simple::command
