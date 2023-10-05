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
  const std::string& getName() const { return name; }
  const std::vector<std::string>& getArgs() const { return args; }
  std::string toString() const;

 private:
  const std::string name;
  const std::vector<std::string> args;
};
}  // namespace networking
}  // namespace redis_simple
