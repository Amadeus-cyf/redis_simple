#include <stdlib.h>

#include <algorithm>
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
  if (cli.Connect("localhost", 8080) == cli::CliStatus::cliErr) {
    RS_LOG_DEBUG("failed to connect to integration server\n");
    return EXIT_FAILURE;
  }

  const std::vector<Case> setup_cases = {
      {"SADD integration_set ele1\r\n", "1\n"},
      {"SADD integration_set ele2\r\n", "1\n"},
      {"SADD integration_set ele3\r\n", "1\n"},
      {"SADD integration_set ele4\r\n", "1\n"},
      {"SADD integration_set ele5\r\n", "1\n"},
      {"SADD integration_set ele6\r\n", "1\n"},
      {"SADD integration_set ele7\r\n", "1\n"},
      {"SCARD integration_set\r\n", "7\n"},
      {"SISMEMBER integration_set ele7\r\n", "1\n"},
      {"SISMEMBER integration_set ele9\r\n", "0\n"},
  };
  for (const Case& test_case : setup_cases) {
    if (!ExpectReply(&cli, test_case)) {
      return EXIT_FAILURE;
    }
  }

  if (!ExpectMembers(
          &cli, "SMEMBERS integration_set\r\n",
          {"ele1", "ele2", "ele3", "ele4", "ele5", "ele6", "ele7"})) {
    return EXIT_FAILURE;
  }

  const std::vector<Case> remove_cases = {
      {"SREM integration_set ele5 ele6 ele7\r\n", "3\n"},
      {"SREM integration_set ele1 ele6 ele7\r\n", "1\n"},
      {"SCARD integration_set\r\n", "3\n"},
      {"SISMEMBER integration_set ele7\r\n", "0\n"},
      {"SISMEMBER integration_set ele9\r\n", "0\n"},
  };
  for (const Case& test_case : remove_cases) {
    if (!ExpectReply(&cli, test_case)) {
      return EXIT_FAILURE;
    }
  }
  return ExpectMembers(&cli, "SMEMBERS integration_set\r\n",
                       {"ele2", "ele3", "ele4"})
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
}  // namespace redis_simple

int main() { return redis_simple::Run(); }
