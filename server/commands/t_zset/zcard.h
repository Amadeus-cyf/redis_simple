#pragma once

#include <string>
#include <vector>

#include "server/commands/command.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_zset {
class ZCardCommand : public Command {
 public:
  ZCardCommand() : Command("ZCARD") {}
  void Exec(Client* const client) const override;

 private:
  struct ZCardArgs {
    std::string key;
  };
  static int ParseArgs(const std::vector<std::string>& args,
                       ZCardArgs* const zcard_args);
  static ssize_t ZCard(const std::shared_ptr<db::RedisDb>& db,
                       const ZCardArgs* args);
};
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
