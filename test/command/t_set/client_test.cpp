#include <vector>

#include "cli/cli.h"
#include "cli/completable_future.h"

namespace redis_simple {
void Run() {
  cli::RedisCli cli;
  if (cli.Connect("localhost", 8080) == cli::CliStatus::cliErr) {
    return;
  }

  const std::string& cmd1 = "SADD key_set ele1\r\n";
  const std::string& cmd2 = "SADD key_set ele2\r\n";
  const std::string& cmd3 = "SADD key_set ele3\r\n";
  const std::string& cmd4 = "SADD key_set ele4 ele5 ele6\r\n";
  const std::string& cmd5 = "SADD key_set ele5 ele6 ele7\r\n";
  const std::string& cmd6 = "SISMEMBER key_set ele7\r\n";
  const std::string& cmd7 = "SISMEMBER key_set ele9\r\n";
  const std::string& cmd8 = "SREM key_set ele5 ele6 ele7\r\n";
  const std::string& cmd9 = "SREM key_set ele1 ele6 ele7\r\n";

  std::vector<std::string> commands = {cmd1, cmd2, cmd3, cmd4, cmd5, cmd6,
                                       cmd7, cmd8, cmd9, cmd6, cmd7};

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
            .ThenApplyAsync(
                [](const std::string& reply) { return "processed: " + reply; })
            .Get();
    printf("after processed, %s\n", applied_str.c_str());
  }
}
}  // namespace redis_simple

int main() { redis_simple::Run(); }
