#pragma once

#include "server/commands/command.h"
#include "server/commands/t_zset/args.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_zset {
class ZRemCommand : public Command {
 public:
  ZRemCommand() : Command("ZREM"){};
  void exec(Client* const client) const override;

 private:
  int parseArgs(const std::vector<std::string>& args,
                ZSetArgs* zset_args) const;
  int genericZRem(std::shared_ptr<const db::RedisDb> db,
                  const ZSetArgs* args) const;
};
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
