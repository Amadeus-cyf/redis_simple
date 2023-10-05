#include "server/commands/command.h"

#include "server/commands/t_string/delete.h"
#include "server/commands/t_string/get.h"
#include "server/commands/t_string/set.h"
#include "server/commands/t_zset/zadd.h"
#include "server/commands/t_zset/zrank.h"
#include "server/commands/t_zset/zrem.h"

namespace redis_simple {
namespace command {
const Command* Command::create(const std::string& name) {
  std::string upper_name;
  std::transform(name.begin(), name.end(), upper_name.begin(), toupper);
  if (name == "GET") {
    return new t_string::GetCommand();
  } else if (name == "SET") {
    return new t_string::SetCommand();
  } else if (name == "DEL") {
    return new t_string::DeleteCommand();
  } else if (name == "ZADD") {
    return new t_zset::ZAddCommand();
  } else if (name == "ZREM") {
    return new t_zset::ZRemCommand();
  } else if (name == "ZRANK") {
    return new t_zset::ZRankCommand();
  } else {
    return nullptr;
  }
}
}  // namespace command
}  // namespace redis_simple
