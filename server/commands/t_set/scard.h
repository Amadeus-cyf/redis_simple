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
  SCardCommand() : Command("SCARD") {}
  void Exec(Client* const client) const override;

 private:
  struct SCardArgs {
    std::string key;
  };
  static int ParseArgs(const std::vector<std::string>& args,
                       SCardArgs* const scard_args);
  static ssize_t SCard(const std::shared_ptr<db::RedisDb>& db,
                       const SCardArgs* args);
};
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
