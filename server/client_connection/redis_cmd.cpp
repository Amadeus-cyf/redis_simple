#include "server/client_connection/redis_cmd.h"

#include <string>
#include <utility>
#include <vector>

namespace redis_simple::client_connection {
RedisCommand::RedisCommand(std::string name, std::vector<std::string> args)
    : name_(std::move(name)), args_(std::move(args)) {}

std::string RedisCommand::String() const {
  size_t command_size = name_.size();
  for (const auto& arg : args_) {
    command_size += 1 + arg.size();
  }
  std::string cmd;
  cmd.reserve(command_size);
  cmd.append(name_);
  for (const std::string& arg : args_) {
    cmd.push_back(' ');
    cmd.append(arg);
  }
  return cmd;
}
}  // namespace redis_simple::client_connection
