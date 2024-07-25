#include <vector>

#include "cli/cli.h"
#include "cli/completable_future.h"

namespace redis_simple {
void Run() {
  cli::RedisCli cli;
  if (cli.Connect("localhost", 8080) == cli::CliStatus::cliErr) {
    return;
  }

  const std::string& cmd1 = "ZADD key1 3.0 ele1\r\n";
  const std::string& cmd2 = "ZADD key1 1 ele1\r\n";
  const std::string& cmd3 = "ZADD key1 1.0000234 ele2\r\n";
  const std::string& cmd4 = "ZRANK key1 ele1\r\n";
  const std::string& cmd5 = "ZRANK key1 ele2\r\n";
  const std::string& cmd6 = "ZRANK key1 ele3\r\n";
  const std::string& cmd7 = "ZCARD key1\r\n";
  const std::string& cmd8 = "ZRANGE key1 0 1\r\n";
  const std::string& cmd9 = "ZRANGE key1 -inf +inf\r\n";
  const std::string& cmd10 = "ZRANGE key1 1.0 2.0 BYSCORE\r\n";
  const std::string& cmd11 = "ZRANGE key1 -inf +inf BYSCORE\r\n";
  const std::string& cmd12 = "ZREM key1 ele1\r\n";
  const std::string& cmd13 = "ZSCORE key1 ele1\r\n";
  const std::string& cmd14 = "ZSCORE key0 ele2\r\n";
  const std::string& cmd15 = "ZSCORE key1 ele2\r\n";
  const std::string& cmd16 = "ZADD key1 0.00000123 ele1\r\n";
  const std::string& cmd17 = "ZADD key1 1234567.123 ele2\r\n";

  std::vector<std::string> commands = {
      cmd1,  cmd2,  cmd3,  cmd4,  cmd5,  cmd6,  cmd7, cmd8,  cmd9,  cmd10,
      cmd12, cmd12, cmd4,  cmd5,  cmd7,  cmd8,  cmd9, cmd10, cmd11, cmd13,
      cmd14, cmd15, cmd16, cmd13, cmd17, cmd15, cmd4, cmd5,  cmd7};

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
