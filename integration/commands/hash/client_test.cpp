#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>
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

bool ExpectPairs(
    cli::RedisCli* cli, const std::string& command,
    std::vector<std::pair<std::string, std::string>> expected_pairs) {
  cli->AddCommand(command);
  const std::vector<std::string> lines = NonEmptyLines(cli->ReadReply());
  if (lines.size() % 2 != 0) {
    RS_LOG_DEBUG("hash pair command returned odd values: %s\n",
                 command.c_str());
    return false;
  }

  std::vector<std::pair<std::string, std::string>> actual_pairs;
  actual_pairs.reserve(lines.size() / 2);
  for (size_t i = 0; i < lines.size(); i += 2) {
    actual_pairs.emplace_back(lines[i], lines[i + 1]);
  }
  std::sort(actual_pairs.begin(), actual_pairs.end());
  std::sort(expected_pairs.begin(), expected_pairs.end());
  if (actual_pairs != expected_pairs) {
    RS_LOG_DEBUG("hash pair command failed: %s\n", command.c_str());
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
      {"HSET integration_hash name redis version 7\r\n", "2\n"},
      {"HSET integration_hash version 8 mode simple\r\n", "1\n"},
      {"HGET integration_hash name\r\n", "redis\n"},
      {"HGET integration_hash version\r\n", "8\n"},
      {"HGET integration_hash missing\r\n", "(nil)\n"},
      {"HGET missing_hash field\r\n", "(nil)\n"},
      {"HEXISTS integration_hash name\r\n", "1\n"},
      {"HEXISTS integration_hash missing\r\n", "0\n"},
      {"HEXISTS missing_hash field\r\n", "0\n"},
      {"HLEN integration_hash\r\n", "3\n"},
      {"HLEN missing_hash\r\n", "0\n"},
      {"HDEL missing_hash field\r\n", "0\n"},
      {"SET hash_wrong_type value\r\n", "1\n"},
      {"HSET hash_wrong_type field value\r\n", "-1\n"},
      {"HGET hash_wrong_type field\r\n", "-1\n"},
      {"HLEN hash_wrong_type\r\n", "-1\n"},
      {"HGETALL hash_wrong_type\r\n", "-1\n"},
  };
  for (const Case& test_case : cases) {
    if (!ExpectReply(&cli, test_case)) {
      return EXIT_FAILURE;
    }
  }

  if (!ExpectPairs(&cli, "HGETALL integration_hash\r\n",
                   {{"name", "redis"}, {"version", "8"}, {"mode", "simple"}})) {
    return EXIT_FAILURE;
  }
  if (!ExpectPairs(&cli, "HGETALL missing_hash\r\n", {})) {
    return EXIT_FAILURE;
  }

  const std::string long_value(65, 'v');
  if (!ExpectReply(
          &cli,
          {"HSET integration_hash_long field " + long_value + "\r\n", "1\n"})) {
    return EXIT_FAILURE;
  }
  if (!ExpectReply(
          &cli, {"HGET integration_hash_long field\r\n", long_value + "\n"})) {
    return EXIT_FAILURE;
  }
  if (!ExpectPairs(&cli, "HGETALL integration_hash_long\r\n",
                   {{"field", long_value}})) {
    return EXIT_FAILURE;
  }

  const std::vector<Case> delete_cases = {
      {"HDEL integration_hash name missing\r\n", "1\n"},
      {"HLEN integration_hash\r\n", "2\n"},
      {"HDEL integration_hash version mode\r\n", "2\n"},
      {"HLEN integration_hash\r\n", "0\n"},
      {"TYPE integration_hash\r\n", "none\n"},
      {"HSET integration_hash recreated value\r\n", "1\n"},
      {"HGET integration_hash recreated\r\n", "value\n"},
  };
  for (const Case& test_case : delete_cases) {
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
