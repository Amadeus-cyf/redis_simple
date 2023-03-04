#include "server/client.h"

namespace redis_simple {
const t_cmd::RedisCmdProc RedisCommand::default_proc = [](Client* c) {
  printf("command processed: %s\n", c->getCmd()->toString().c_str());
  c->addReply("PONG");
  c->sendReply();
};

RedisCommand::RedisCommand(const std::string& cmd_name,
                           const std::vector<std::string>& cmd_args,
                           t_cmd::RedisCmdProc proc)
    : name(cmd_name), args(cmd_args), cmd_proc(proc) {}

std::string RedisCommand::toString() const {
  std::string cmd = name;
  for (const std::string& arg : args) {
    cmd.push_back(' ');
    cmd.append(arg);
  }
  return cmd;
}

int RedisCommand::exec(Client* c) const {
  if (!cmd_proc) {
    printf("no proc, call default proc\n");
    default_proc(c);
  } else {
    cmd_proc(c);
  }
  return 0;
}
}  // namespace redis_simple
