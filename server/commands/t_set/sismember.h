#pragma once

#include <string>
#include <vector>

#include "server/commands/t_set/sadd.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_set {
class SIsMemberCommand : public Command {
 public:
  SIsMemberCommand() : Command("SISMEMBER") {}
  void Exec(Client* const client) const override;

 private:
  struct SIsMemberArgs {
    std::string key;
    std::string element;
  };
  static int ParseArgs(const std::vector<std::string>& args,
                       SIsMemberArgs* const sismember_args);
  static int SIsMember(const std::shared_ptr<db::RedisDb>& db,
                       const SIsMemberArgs* args);
};
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
