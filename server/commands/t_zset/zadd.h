#pragma once

#include "server/commands/command.h"
#include "server/commands/t_zset/args.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_zset {
class ZAddCommand : public Command {
 public:
  ZAddCommand() : Command("ZADD"){};
  void exec(Client* const client) const override;

 private:
  int parseArgs(const std::vector<std::string>& args,
                ZSetArgs* zset_args) const;
  int genericZAdd(const db::RedisDb* db, const ZSetArgs* args) const;
};
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
