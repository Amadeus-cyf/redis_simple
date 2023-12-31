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

  cli.AddCommand(cmd1);
  cli.AddCommand(cmd2);
  cli.AddCommand(cmd3);
  cli.AddCommand(cmd4);
  cli.AddCommand(cmd5);
  cli.AddCommand(cmd6);
  cli.AddCommand(cmd7);
  cli.AddCommand(cmd8);
  cli.AddCommand(cmd9);
  cli.AddCommand(cmd10);
  cli.AddCommand(cmd10);
  cli.AddCommand(cmd3);
  cli.AddCommand(cmd6);
  cli.AddCommand(cmd7);
  cli.AddCommand(cmd8);
  cli.AddCommand(cmd9);

  auto r1 = cli.GetReplyAsync();
  auto r2 = cli.GetReplyAsync();
  auto r3 = cli.GetReplyAsync();
  auto r4 = cli.GetReplyAsync();
  auto r5 = cli.GetReplyAsync();
  auto r6 = cli.GetReplyAsync();
  auto r7 = cli.GetReplyAsync();
  auto r8 = cli.GetReplyAsync();
  auto r9 = cli.GetReplyAsync();
  auto r10 = cli.GetReplyAsync();
  auto r11 = cli.GetReplyAsync();
  auto r12 = cli.GetReplyAsync();
  auto r13 = cli.GetReplyAsync();
  auto r14 = cli.GetReplyAsync();
  auto r15 = cli.GetReplyAsync();
  auto r16 = cli.GetReplyAsync();

  const std::string& applied_str1 =
      r1.ThenApply([](const std::string& reply) {
          printf("receive resp 1: %s end\n", reply.c_str());
          return reply;
        })
          .ThenApply(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str1.c_str());
  ;
  const std::string& applied_str2 =
      r2.ThenApply([](const std::string& reply) {
          printf("receive resp 2: %s end\n", reply.c_str());
          return reply;
        })
          .ThenApply(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str2.c_str());

  const std::string& applied_str3 =
      r3.ThenApply([](const std::string& reply) {
          printf("receive resp 3: %s end\n", reply.c_str());
          return reply;
        })
          .ThenApply(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str3.c_str());

  const std::string& applied_str4 =
      r4.ThenApply([](const std::string& reply) {
          printf("receive resp 4: %s end\n", reply.c_str());
          return reply;
        })
          .ThenApply(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str4.c_str());

  const std::string& applied_str5 =
      r5.ThenApplyAsync([](const std::string& reply) {
          printf("receive resp 5: %s end\n", reply.c_str());
          return reply;
        })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str5.c_str());

  const std::string& applied_str6 =
      r6.ThenApplyAsync([](const std::string& reply) {
          printf("receive resp 6: %s end\n", reply.c_str());
          return reply;
        })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str6.c_str());

  const std::string& applied_str7 =
      r7.ThenApplyAsync([](const std::string& reply) {
          printf("receive resp 7: %s end\n", reply.c_str());
          return reply;
        })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str7.c_str());

  const std::string& applied_str8 =
      r8.ThenApplyAsync([](const std::string& reply) {
          printf("receive resp 8: %s end\n", reply.c_str());
          return reply;
        })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str8.c_str());

  const std::string& applied_str9 =
      r9.ThenApplyAsync([](const std::string& reply) {
          printf("receive resp 9: %s end\n", reply.c_str());
          return reply;
        })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str9.c_str());

  const std::string& applied_str10 =
      r10.ThenApplyAsync([](const std::string& reply) {
           printf("receive resp 10: %s end\n", reply.c_str());
           return reply;
         })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str10.c_str());

  const std::string& applied_str11 =
      r11.ThenApplyAsync([](const std::string& reply) {
           printf("receive resp 11: %s end\n", reply.c_str());
           return reply;
         })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str11.c_str());

  const std::string& applied_str12 =
      r12.ThenApplyAsync([](const std::string& reply) {
           printf("receive resp 12: %s end\n", reply.c_str());
           return reply;
         })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str12.c_str());

  const std::string& applied_str13 =
      r13.ThenApplyAsync([](const std::string& reply) {
           printf("receive resp 13: %s end\n", reply.c_str());
           return reply;
         })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str13.c_str());

  const std::string& applied_str14 =
      r14.ThenApplyAsync([](const std::string& reply) {
           printf("receive resp 14: %s end\n", reply.c_str());
           return reply;
         })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str14.c_str());

  const std::string& applied_str15 =
      r15.ThenApplyAsync([](const std::string& reply) {
           printf("receive resp 15: %s end\n", reply.c_str());
           return reply;
         })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str15.c_str());

  const std::string& applied_str16 =
      r16.ThenApplyAsync([](const std::string& reply) {
           printf("receive resp 16: %s end\n", reply.c_str());
           return reply;
         })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed, %s\n", applied_str16.c_str());
}
}  // namespace redis_simple

int main() { redis_simple::Run(); }
