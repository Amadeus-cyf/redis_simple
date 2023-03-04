#pragma once

#include <string>
#include <vector>

#include "server/t_string.h"

namespace redis_simple {
class Client;
class RedisCommand {
 public:
  explicit RedisCommand(
      const std::string& name,
      const std::vector<std::string>& args = std::vector<std::string>{},
      t_cmd::RedisCmdProc proc = nullptr);
  const std::string& getName() const { return name; }
  const std::vector<std::string>& getArgs() const { return args; }
  std::string toString() const;
  int exec(Client* c) const;

 private:
  static const t_cmd::RedisCmdProc default_proc;
  const std::string name;
  const std::vector<std::string> args;
  t_cmd::RedisCmdProc cmd_proc;
};

}  // namespace redis_simple
