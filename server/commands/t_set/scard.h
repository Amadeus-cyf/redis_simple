#pragma once

#include <string>
#include <vector>

#include "server/commands/command.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_set {
class SCardCommand : public Command {
 public:
  SCardCommand() : Command("SCARD"){};
  void Exec(Client* const client) const override;

 private:
  struct SAddArgs {
    std::string key;
  };
  int ParseArgs(const std::vector<std::string>& args,
                SAddArgs* const sadd_args) const;
  ssize_t SCard(std::shared_ptr<const db::RedisDb> db,
                const SAddArgs* args) const;
};
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
