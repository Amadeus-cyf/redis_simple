#include "cli/cli.h"
#include "cli/completable_future.h"

namespace redis_simple {
void Run() {
  cli::RedisCli cli;
  cli.Connect("localhost", 8081);

  const std::string& cmd1 = "SET key val\r\n";
  const std::string& cmd2 = "GET key\r\n";
  std::vector<std::string> commands = {
      cmd1,
      cmd2,
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
