#pragma once

#include "server/commands/command.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_zset {
class ZScoreCommand : public Command {
 public:
  ZScoreCommand() : Command("ZSCORE"){};
  void Exec(Client* const client) const override;

 private:
  struct ZScoreArgs {
    std::string key;
    std::string element;
  };
  int ParseArgs(const std::vector<std::string>& args,
                ZScoreArgs* const zscore_args) const;
  const std::optional<double> ZScore(std::shared_ptr<const db::RedisDb> db,
                                     const ZScoreArgs* args) const;
};
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
