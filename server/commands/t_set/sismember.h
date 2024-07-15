#pragma once

#include <string>
#include <vector>

#include "server/client.h"
#include "server/commands/t_set/sadd.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_set {
class SIsMemberCommand : public Command {
 public:
  SIsMemberCommand() : Command("SISMEMBER"){};
  void Exec(Client* const client) const override;

 private:
  struct SIsMemberArgs {
    std::string key;
    std::string element;
  };
  int ParseArgs(const std::vector<std::string>& args,
                SIsMemberArgs* const sismember_args) const;
  int SIsMember(std::shared_ptr<const db::RedisDb> db,
                const SIsMemberArgs* args) const;
};
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
