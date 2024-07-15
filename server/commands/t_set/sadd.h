#pragma once

#include <string>
#include <vector>

#include "server/commands/command.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_set {
class SAddCommand : public Command {
 public:
  SAddCommand() : Command("SADD"){};
  void Exec(Client* const client) const override;

 private:
  struct SAddArgs {
    std::string key;
    std::vector<std::string> elements;
  };
  int ParseArgs(const std::vector<std::string>& args,
                SAddArgs* const sadd_args) const;
  int SAdd(std::shared_ptr<const db::RedisDb> db, const SAddArgs* args) const;
};
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
