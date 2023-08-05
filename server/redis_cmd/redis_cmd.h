#pragma once

#include <string>
#include <vector>

namespace redis_simple {
class Client;

class RedisCommand {
 public:
  using RedisCmdProc = void (*)(Client* const);
  explicit RedisCommand(
      const std::string& name,
      const std::vector<std::string>& args = std::vector<std::string>{},
      RedisCmdProc proc = nullptr);
  const std::string& getName() const { return name; }
  const std::vector<std::string>& getArgs() const { return args; }
  std::string toString() const;
  int exec(Client* const c) const;

 private:
  static const RedisCmdProc default_proc;
  const std::string name;
  const std::vector<std::string> args;
  RedisCmdProc cmd_proc;
};
}  // namespace redis_simple
