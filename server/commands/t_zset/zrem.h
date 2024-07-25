#pragma once

#include <vector>

#include "server/commands/command.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_zset {
class ZRemCommand : public Command {
 public:
  ZRemCommand() : Command("ZREM"){};
  void Exec(Client* const client) const override;

 private:
  struct ZRemArgs {
    std::string key;
    std::vector<std::string> elements;
  };
  int ParseArgs(const std::vector<std::string>& args,
                ZRemArgs* const zset_args) const;
  int ZRem(std::shared_ptr<const db::RedisDb> db, const ZRemArgs* args) const;
};
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
