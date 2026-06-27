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
  SAddCommand() : Command("SADD") {}
  void Exec(Client* const client) const override;

 private:
  struct SAddArgs {
    std::string key;
    std::vector<std::string> elements;
  };
  static int ParseArgs(const std::vector<std::string>& args,
                       SAddArgs* const sadd_args);
  static int SAdd(const std::shared_ptr<db::RedisDb>& db, const SAddArgs* args);
};
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
