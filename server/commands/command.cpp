#include "server/commands/command.h"

#include "server/commands/t_string/delete.h"
#include "server/commands/t_string/get.h"
#include "server/commands/t_string/set.h"
#include "server/commands/t_zset/zadd.h"
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
        {"ZADD", std::make_shared<const t_zset::ZAddCommand>()},
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
