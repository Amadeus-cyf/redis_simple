#include "cli/cli.h"
#include "cli/completable_future.h"

namespace redis_simple {
void Run() {
  cli::RedisCli cli;
  cli.Connect("localhost", 8081);

  const std::string& cmd1 = "ZADD key1 ele1 1.0\r\n";
  const std::string& cmd2 = "ZADD key1 ele2 1.0\r\n";
  const std::string& cmd3 = "ZRANK key1 ele1\r\n";
  const std::string& cmd4 = "ZRANK key1 ele2\r\n";
  const std::string& cmd5 = "ZRANK key1 ele3\r\n";
  const std::string& cmd6 = "ZRANGE key1 0 1\r\n";
  const std::string& cmd7 = "ZRANGE key1 -inf +inf\r\n";
  const std::string& cmd8 = "ZRANGE key1 1.0 2.0 BYSCORE\r\n";
  const std::string& cmd9 = "ZRANGE key1 -inf +inf BYSCORE\r\n";
  const std::string& cmd10 = "ZREM key1 ele1\r\n";

  std::vector<std::string> commands = {
      cmd1, cmd2,  cmd3,  cmd4, cmd5, cmd6, cmd7, cmd8,
      cmd9, cmd10, cmd10, cmd3, cmd6, cmd7, cmd8, cmd9,
  };

  for (const std::string& command : commands) {
    cli.AddCommand(command);
  }

  for (const std::string& command : commands) {
    const std::string& applied_str =
        cli.GetReplyAsync()
            .ThenApply([](const std::string& reply) {
              printf("receive resp: %s end\n", reply.c_str());
              return reply;
            })
            .ThenApply(
                [](const std::string& reply) { return "processed: " + reply; })
            .Get();
    printf("after processed, %s\n", applied_str.c_str());
  }
}
}  // namespace redis_simple

int main() { redis_simple::Run(); }
