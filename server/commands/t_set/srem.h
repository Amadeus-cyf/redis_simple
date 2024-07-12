#pragma once

#include <string>
#include <vector>

#include "server/commands/command.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_set {
struct SRemArgs {
  std::string key;
  std::vector<std::string> elements;
};

class SRemCommand : public Command {
 public:
  SRemCommand() : Command("SREM"){};
  void Exec(Client* const client) const override;

 private:
  int ParseArgs(const std::vector<std::string>& args,
                SRemArgs* const srem_args) const;
  int SRem(std::shared_ptr<const db::RedisDb> db, const SRemArgs* args) const;
};
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
