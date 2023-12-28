#include "cli/cli.h"
#include "cli/completable_future.h"

namespace redis_simple {
void Run() {
  cli::RedisCli cli;
  cli.Connect("localhost", 8081);

  const std::string& cmd1 = "SET key val\r\n";
  const std::string& cmd2 = "GET key\r\n";
  cli.AddCommand(cmd1);
  cli.AddCommand(cmd2);

  auto r1 = cli.GetReplyAsync();
  auto r2 = cli.GetReplyAsync();

  const std::string& applied_str1 =
      r1.ThenApply([](const std::string& reply) {
          printf("receive resp: %s end\n", reply.c_str());
          return reply;
        })
          .ThenApply(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str1.c_str());

  const std::string& applied_str2 =
      r2.ThenApply([](const std::string& reply) {
          printf("receive resp: %s end\n", reply.c_str());
          return reply;
        })
          .ThenApply(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str2.c_str());
}
}  // namespace redis_simple

int main() { redis_simple::Run(); }
