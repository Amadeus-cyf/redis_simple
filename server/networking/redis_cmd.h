#pragma once

#include <string>
#include <vector>

namespace redis_simple {
class Client;

namespace networking {
/* for testing */
class RedisCommand {
 public:
  explicit RedisCommand(
      const std::string& name,
      const std::vector<std::string>& args = std::vector<std::string>{});
  const std::string& Name() const { return name; }
  const std::vector<std::string>& Args() const { return args; }
  std::string String() const;

 private:
  const std::string name;
  const std::vector<std::string> args;
};
}  // namespace networking
}  // namespace redis_simple
