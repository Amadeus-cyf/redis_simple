#pragma once

#include "server/commands/command.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_zset {
class ZAddCommand : public Command {
 public:
  ZAddCommand() : Command("ZADD"){};
  void Exec(Client* const client) const override;

 private:
  struct ZAddArgs {
    std::string key;
    std::vector<std::pair<std::string, double>> ele_score_list;
  };
  int ParseArgs(const std::vector<std::string>& args,
                ZAddArgs* const zset_args) const;
  int ZAdd(std::shared_ptr<const db::RedisDb> db, const ZAddArgs* args) const;
};
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
