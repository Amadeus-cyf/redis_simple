#include "server/networking/redis_cmd.h"

#include "server/client.h"

namespace redis_simple {
namespace networking {
RedisCommand::RedisCommand(const std::string& name,
                           const std::vector<std::string>& args)
    : name_(name), args_(args) {}

std::string RedisCommand::String() const {
  std::string cmd = name_;
  for (const std::string& arg : args_) {
    cmd.push_back(' ');
    cmd.append(arg);
  }
  return cmd;
}
}  // namespace networking
}  // namespace redis_simple
