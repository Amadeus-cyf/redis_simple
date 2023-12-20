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
  void exec(Client* const client) const override;

 private:
  int parseArgs(const std::vector<std::string>& args, StrArgs* str_args) const;
  int genericDelete(std::shared_ptr<const db::RedisDb> db,
                    const StrArgs* args) const;
};
}  // namespace t_string
}  // namespace command
}  // namespace redis_simple
