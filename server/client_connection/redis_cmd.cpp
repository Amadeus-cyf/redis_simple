#include "server/client_connection/redis_cmd.h"

#include <utility>

#include "server/client.h"

namespace redis_simple::client_connection {
RedisCommand::RedisCommand(std::string name,
                           const std::vector<std::string>& args)
    : name_(std::move(name)), args_(args) {}

std::string RedisCommand::String() const {
  std::string cmd = name_;
  for (const std::string& arg : args_) {
    cmd.push_back(' ');
    cmd.append(arg);
  }
  return cmd;
}
}  // namespace redis_simple::client_connection
