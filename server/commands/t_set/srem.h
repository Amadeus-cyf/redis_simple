#pragma once

#include <string>
#include <vector>

#include "server/commands/command.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_set {
class SRemCommand : public Command {
 public:
  SRemCommand() : Command("SREM") {}
  void Exec(Client* const client) const override;

 private:
  struct SRemArgs {
    std::string key;
    std::vector<std::string> elements;
  };
  static int ParseArgs(const std::vector<std::string>& args,
                       SRemArgs* const srem_args);
  static int SRem(const std::shared_ptr<db::RedisDb>& db, const SRemArgs* args);
};
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
