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
  const std::string reply = cli->GetReply();
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
      {"SET string_key val 1000\r\n", "1\n"},
      {"GET string_key\r\n", "val\n"},
      {"GET missing_string_key\r\n", "(nil)\n"},
      {"SET string_key val1 3000\r\n", "1\n"},
      {"GET string_key\r\n", "val1\n"},
      {"DEL string_key\r\n", "1\n"},
      {"DEL missing_string_key\r\n", "0\n"},
      {"GET string_key\r\n", "(nil)\n"},
      {"SET string_key_1 value1 1000\r\n", "1\n"},
      {"SET string_key_2 value2 1000\r\n", "1\n"},
      {"DEL string_key_1 missing_string_key string_key_2\r\n", "2\n"},
      {"GET string_key_1\r\n", "(nil)\n"},
      {"GET string_key_2\r\n", "(nil)\n"},
      {"DEL\r\n", "-1\n"},
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
