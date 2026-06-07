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
  ZCardCommand() : Command("ZCARD"){};
  void Exec(Client* const client) const override;

 private:
  struct ZCardArgs {
    std::string key;
  };
  int ParseArgs(const std::vector<std::string>& args,
                ZCardArgs* const zcard_args) const;
  ssize_t ZCard(std::shared_ptr<db::RedisDb> db, const ZCardArgs* args) const;
};
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
