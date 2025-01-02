#pragma once

#include <string>
#include <vector>

namespace redis_simple {
class Client;

namespace networking {
// For testing
class RedisCommand {
 public:
  explicit RedisCommand(
      const std::string& name,
      const std::vector<std::string>& args = std::vector<std::string>{});
  const std::string& Name() const { return name_; }
  const std::vector<std::string>& Args() const { return args_; }
  std::string String() const;

 private:
  const std::string name_;
  const std::vector<std::string> args_;
};
}  // namespace networking
}  // namespace redis_simple
