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

namespace redis_simple {
namespace command {
const std::unordered_map<std::string, std::shared_ptr<const Command>>&
    Command::cmdmap = {
        {"GET", std::make_shared<const t_string::GetCommand>()},
        {"SET", std::make_shared<const t_string::SetCommand>()},
        {"DEL", std::make_shared<const t_string::DeleteCommand>()},
        {"SADD", std::make_shared<const t_set::SAddCommand>()},
        {"SCARD", std::make_shared<const t_set::SCardCommand>()},
        {"SREM", std::make_shared<const t_set::SRemCommand>()},
        {"SMEMBERS", std::make_shared<const t_set::SMembersCommand>()},
        {"SISMEMBER", std::make_shared<const t_set::SIsMemberCommand>()},
        {"ZADD", std::make_shared<const t_zset::ZAddCommand>()},
        {"ZCARD", std::make_shared<const t_zset::ZCardCommand>()},
        {"ZREM", std::make_shared<const t_zset::ZRemCommand>()},
        {"ZRANK", std::make_shared<const t_zset::ZRankCommand>()},
        {"ZRANGE", std::make_shared<const t_zset::ZRangeCommand>()},
};

std::weak_ptr<const Command> Command::Create(const std::string& name) {
  std::string upper_name;
  std::transform(name.begin(), name.end(), upper_name.begin(), toupper);
  try {
    return cmdmap.at(name);
  } catch (const std::out_of_range& e) {
    return std::shared_ptr<const Command>(nullptr);
  }
}
}  // namespace command
}  // namespace redis_simple
