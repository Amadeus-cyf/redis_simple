#pragma once

#include "server/commands/command.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_zset {
class ZScoreCommand : public Command {
 public:
  ZScoreCommand() : Command("ZSCORE") {}
  void Exec(Client* const client) const override;

 private:
  struct ZScoreArgs {
    std::string key;
    std::string element;
  };
  static int ParseArgs(const std::vector<std::string>& args,
                       ZScoreArgs* const zscore_args);
  static std::optional<double> ZScore(const std::shared_ptr<db::RedisDb>& db,
                                      const ZScoreArgs* args);
};
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
