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
  const std::string reply = cli->ReadReply();
  if (reply != test_case.expected_reply) {
    RS_LOG_DEBUG("command failed: %s expected: %s actual: %s\n",
                 test_case.command.c_str(), test_case.expected_reply.c_str(),
                 reply.c_str());
    return false;
  }
  return true;
}

bool ExpectLines(cli::RedisCli* cli, const std::string& command,
                 const std::vector<std::string>& expected_lines) {
  cli->AddCommand(command);
  const std::vector<std::string> actual_lines = NonEmptyLines(cli->ReadReply());
  if (actual_lines != expected_lines) {
    RS_LOG_DEBUG("line command failed: %s\n", command.c_str());
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

  const std::vector<Case> setup_cases = {
      {"RPUSH integration_list one two three\r\n", "3\n"},
      {"LLEN integration_list\r\n", "3\n"},
      {"LPUSH integration_list zero\r\n", "4\n"},
      {"LPOP integration_list\r\n", "zero\n"},
      {"RPOP integration_list\r\n", "three\n"},
      {"LLEN integration_list\r\n", "2\n"},
  };
  for (const Case& test_case : setup_cases) {
    if (!ExpectReply(&cli, test_case)) {
      return EXIT_FAILURE;
    }
  }

  if (!ExpectLines(&cli, "LRANGE integration_list 0 -1\r\n", {"one", "two"})) {
    return EXIT_FAILURE;
  }
  if (!ExpectLines(&cli, "LRANGE integration_list -2 -1\r\n", {"one", "two"})) {
    return EXIT_FAILURE;
  }
  if (!ExpectLines(&cli, "LRANGE integration_list 10 20\r\n", {})) {
    return EXIT_FAILURE;
  }

  const std::vector<Case> missing_and_type_cases = {
      {"LPOP integration_list\r\n", "one\n"},
      {"LPOP integration_list\r\n", "two\n"},
      {"LPOP integration_list\r\n", "(nil)\n"},
      {"LLEN integration_list\r\n", "0\n"},
      {"RPUSH integration_list recreated\r\n", "1\n"},
      {"LPOP integration_list\r\n", "recreated\n"},
      {"SET list_string value 1000\r\n", "1\n"},
      {"LLEN list_string\r\n", "-1\n"},
      {"LPUSH list_string value\r\n", "-1\n"},
      {"LRANGE list_string 0 -1\r\n", "-1\n"},
  };
  for (const Case& test_case : missing_and_type_cases) {
    if (!ExpectReply(&cli, test_case)) {
      return EXIT_FAILURE;
    }
  }

  return ExpectLines(&cli, "LRANGE missing_list 0 -1\r\n", {}) ? EXIT_SUCCESS
                                                               : EXIT_FAILURE;
}
}  // namespace redis_simple

int main() {
  try {
    return redis_simple::Run();
  } catch (...) {
    return EXIT_FAILURE;
  }
}
