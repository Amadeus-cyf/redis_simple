#include "server/networking/redis_cmd.h"

#include "server/client.h"

namespace redis_simple {
namespace networking {
RedisCommand::RedisCommand(const std::string& cmd_name,
                           const std::vector<std::string>& cmd_args)
    : name(cmd_name), args(cmd_args) {}

std::string RedisCommand::String() const {
  std::string cmd = name;
  for (const std::string& arg : args) {
    cmd.push_back(' ');
    cmd.append(arg);
  }
  return cmd;
}
}  // namespace networking
}  // namespace redis_simple
