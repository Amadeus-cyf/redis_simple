#include "server/redis_cmd/redis_cmd.h"

#include "server/client.h"

namespace redis_simple {
const RedisCommand::RedisCmdProc RedisCommand::default_proc =
    [](Client* const c) {
      printf("command processed: %s\n", c->getCmd()->toString().c_str());
      c->addReply("PONG");
      c->sendReply();
    };

RedisCommand::RedisCommand(const std::string& cmd_name,
                           const std::vector<std::string>& cmd_args,
                           RedisCmdProc proc)
    : name(cmd_name), args(cmd_args), cmd_proc(proc) {}

std::string RedisCommand::toString() const {
  std::string cmd = name;
  for (const std::string& arg : args) {
    cmd.push_back(' ');
    cmd.append(arg);
  }
  return cmd;
}

int RedisCommand::exec(Client* const c) const {
  if (!cmd_proc) {
    printf("no proc, call default proc\n");
    default_proc(c);
  } else {
    cmd_proc(c);
  }
  return 0;
}
}  // namespace redis_simple
