#include <vector>

#include "cli/cli.h"
#include "cli/completable_future.h"

namespace redis_simple {
void Run() {
  cli::RedisCli cli;
  if (cli.Connect("localhost", 8080) == cli::CliStatus::cliErr) {
    return;
  }

  const std::string& cmd0 = "SET key val 1000\r\n";
  const std::string& cmd1 = "GET key\r\n";
  const std::string& cmd2 = "GET key2\r\n";
  const std::string& cmd3 = "SET key val1 3000\r\n";
  std::vector<std::string> commands = {
      cmd0, cmd1, cmd2, cmd3, cmd0,
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
