#pragma once

#include <string>
#include <vector>

#include "server/commands/command.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_set {
class SMembersCommand : public Command {
 public:
  SMembersCommand() : Command("SMEMBERS"){};
  void Exec(Client* const client) const override;

 private:
  struct SMembersArgs {
    std::string key;
  };
  int ParseArgs(const std::vector<std::string>& args,
                SMembersArgs* const smembers_args) const;
  int SMembers(std::shared_ptr<const db::RedisDb> db, const SMembersArgs* args,
               std::vector<std::string>& members) const;
};
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
