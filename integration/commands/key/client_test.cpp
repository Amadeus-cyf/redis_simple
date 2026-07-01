#include <cstdlib>
#include <string>
#include <vector>

#include "cli/cli.h"

namespace redis_simple {
namespace {
struct Case {
  std::string command;
  std::string expected_reply;
};

bool ExpectReply(cli::RedisCli* cli, const Case& test_case) {
  cli->AddCommand(test_case.command);
  const std::string reply = cli->ReadReply();
  if (reply != test_case.expected_reply) {
    RS_LOG_DEBUG("command failed: %s expected: %s actual: %s\n",
                 test_case.command.c_str(), test_case.expected_reply.c_str(),
                 reply.c_str());
    return false;
  }
  return true;
}
}  // namespace

int Run() {
  cli::RedisCli cli;
  if (cli.Connect("localhost", 8080) == cli::CliStatus::kError) {
    RS_LOG_DEBUG("failed to connect to integration server\n");
    return EXIT_FAILURE;
  }

  const std::vector<Case> cases = {
      {"TYPE missing_key\r\n", "none\n"},
      {"EXISTS missing_key\r\n", "0\n"},
      {"SET key_string value\r\n", "1\n"},
      {"SADD key_set member\r\n", "1\n"},
      {"RPUSH key_list one two\r\n", "2\n"},
      {"ZADD key_zset 1.0 member\r\n", "1\n"},
      {"TYPE key_string\r\n", "string\n"},
      {"TYPE key_set\r\n", "set\n"},
      {"TYPE key_list\r\n", "list\n"},
      {"TYPE key_zset\r\n", "zset\n"},
      {"EXISTS key_string key_set key_list key_zset missing_key\r\n", "4\n"},
      {"DEL key_string missing_key\r\n", "1\n"},
      {"TYPE key_string\r\n", "none\n"},
      {"EXISTS key_string key_set\r\n", "1\n"},
      {"DEL key_set key_list key_zset\r\n", "3\n"},
      {"EXISTS key_set key_list key_zset\r\n", "0\n"},
      {"TYPE key_set\r\n", "none\n"},
  };

  for (const Case& test_case : cases) {
    if (!ExpectReply(&cli, test_case)) {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
}  // namespace redis_simple

int main() {
  try {
    return redis_simple::Run();
  } catch (...) {
    return EXIT_FAILURE;
  }
}
