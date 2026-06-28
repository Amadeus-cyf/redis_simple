#include <algorithm>
#include <cstdlib>
#include <sstream>
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

std::vector<std::string> NonEmptyLines(const std::string& reply) {
  std::stringstream stream(reply);
  std::string line;
  std::vector<std::string> lines;
  while (std::getline(stream, line)) {
    if (!line.empty()) {
      lines.push_back(line);
    }
  }
  return lines;
}

bool ExpectMembers(cli::RedisCli* cli, const std::string& command,
                   std::vector<std::string> expected_members) {
  cli->AddCommand(command);
  std::vector<std::string> actual_members = NonEmptyLines(cli->GetReply());
  std::sort(actual_members.begin(), actual_members.end());
  std::sort(expected_members.begin(), expected_members.end());
  if (actual_members != expected_members) {
    RS_LOG_DEBUG("member command failed: %s\n", command.c_str());
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
      {"ZADD integration_zset 3.0 ele1\r\n", "1\n"},
      {"ZADD integration_zset 1 ele1\r\n", "0\n"},
      {"ZADD integration_zset 1.0000234 ele2\r\n", "1\n"},
      {"ZRANK integration_zset ele1\r\n", "0\n"},
      {"ZRANK integration_zset ele2\r\n", "1\n"},
      {"ZRANK integration_zset ele3\r\n", "(nil)\n"},
      {"ZCARD integration_zset\r\n", "2\n"},
      {"ZCARD missing_zset\r\n", "0\n"},
      {"ZREM missing_zset ele1 ele2\r\n", "0\n"},
      {"ZRANGE integration_zset 0 1 WTHSCORES\r\n", "-1\n"},
      {"ZRANGE integration_zset 0 1 LIMIT 0\r\n", "-1\n"},
  };
  for (const Case& test_case : cases) {
    if (!ExpectReply(&cli, test_case)) {
      return EXIT_FAILURE;
    }
  }

  if (!ExpectMembers(&cli, "ZRANGE integration_zset 0 1\r\n",
                     {"ele1", "ele2"})) {
    return EXIT_FAILURE;
  }
  if (!ExpectMembers(&cli, "ZRANGE integration_zset 1.0 2.0 BYSCORE\r\n",
                     {"ele1", "ele2"})) {
    return EXIT_FAILURE;
  }
  if (!ExpectReply(&cli, {"ZRANGE integration_zset 0 1 WITHSCORES\r\n",
                          "ele1\n1\nele2\n1.0000234\n\n\n"})) {
    return EXIT_FAILURE;
  }
  if (!ExpectMembers(&cli, "ZRANGE missing_zset 0 -1\r\n", {})) {
    return EXIT_FAILURE;
  }

  const std::vector<Case> remove_cases = {
      {"ZREM integration_zset ele1\r\n", "1\n"},
      {"ZREM integration_zset ele1\r\n", "0\n"},
      {"ZRANK integration_zset ele1\r\n", "(nil)\n"},
      {"ZRANK integration_zset ele2\r\n", "0\n"},
      {"ZCARD integration_zset\r\n", "1\n"},
      {"ZSCORE integration_zset ele1\r\n", "(nil)\n"},
      {"ZSCORE missing_zset ele2\r\n", "(nil)\n"},
  };
  for (const Case& test_case : remove_cases) {
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
