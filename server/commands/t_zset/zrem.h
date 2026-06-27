#pragma once

#include <vector>

#include "server/commands/command.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_zset {
class ZRemCommand : public Command {
 public:
  ZRemCommand() : Command("ZREM") {}
  void Exec(Client* const client) const override;

 private:
  struct ZRemArgs {
    std::string key;
    std::vector<std::string> elements;
  };
  static int ParseArgs(const std::vector<std::string>& args,
                       ZRemArgs* const zset_args);
  static int ZRem(const std::shared_ptr<db::RedisDb>& db, const ZRemArgs* args);
};
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
