#pragma once

#include "server/commands/command.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_zset {
class ZRankCommand : public Command {
 public:
  ZRankCommand() : Command("ZRANK") {}
  void Exec(Client* const client) const override;

 private:
  struct ZRankArgs {
    std::string key;
    std::string ele;
  };
  static int ParseArgs(const std::vector<std::string>& args,
                       ZRankArgs* const zset_args);
  static std::optional<size_t> ZRank(const std::shared_ptr<db::RedisDb>& db,
                                     const ZRankArgs* args);
};
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
