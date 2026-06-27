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
        {"ZSCORE", std::make_shared<const t_zset::ZScoreCommand>()},
};

std::weak_ptr<const Command> Command::Create(const std::string& name) {
  auto upper_name = name;
  utils::ToUppercase(upper_name);
  try {
    return cmdmap.at(upper_name);
  } catch (const std::out_of_range&) {
    return std::shared_ptr<const Command>(nullptr);
  }
}
}  // namespace redis_simple::command
