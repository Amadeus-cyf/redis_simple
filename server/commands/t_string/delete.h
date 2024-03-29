#pragma once

#include "server/commands/command.h"
#include "server/commands/t_string/args.h"
#include "server/db/db.h"

namespace redis_simple {
namespace command {
namespace t_string {
class DeleteCommand : public Command {
 public:
  DeleteCommand() : Command("DEL"){};
  void Exec(Client* const client) const override;

 private:
  int ParseArgs(const std::vector<std::string>& args, StrArgs* str_args) const;
  int Delete(std::shared_ptr<const db::RedisDb> db, const StrArgs* args) const;
};
}  // namespace t_string
}  // namespace command
}  // namespace redis_simple
