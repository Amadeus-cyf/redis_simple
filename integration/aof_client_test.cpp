#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "cli/cli.h"
#include "logging/logger.h"

namespace redis_simple {
namespace {
constexpr int kAofPort = 18081;

struct Case {
  std::string command;
  std::string expected_reply;
};

bool ExpectReply(cli::RedisCli* const cli, const Case& test_case) {
  cli->AddCommand(test_case.command);
  const std::string reply = cli->ReadReply();
  if (reply == test_case.expected_reply) {
    return true;
  }
  RS_LOG_DEBUG("AOF command failed: %s expected: %s actual: %s\n",
               test_case.command.c_str(), test_case.expected_reply.c_str(),
               reply.c_str());
  return false;
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

bool WriteDataset(cli::RedisCli* const cli) {
  const std::vector<Case> cases = {
      {"SET aof_string value\r\n", "OK\n"},
      {"SET aof_ttl live PX 60000\r\n", "OK\n"},
      {"SET aof_expired old PX 100\r\n", "OK\n"},
      {"APPEND aof_expired _suffix\r\n", "10\n"},
      {"HSET aof_hash field value\r\n", "1\n"},
      {"RPUSH aof_list one two\r\n", "2\n"},
      {"SADD aof_set first second\r\n", "2\n"},
      {"ZADD aof_zset 1 member\r\n", "1\n"},
      {"SET aof_deleted gone\r\n", "OK\n"},
      {"DEL aof_deleted\r\n", "1\n"},
      {"HSET aof_string field rejected\r\n",
       "WRONGTYPE Operation against a key holding the wrong kind of value\n"},
  };
  if (!std::all_of(cases.begin(), cases.end(), [cli](const auto& test_case) {
        return ExpectReply(cli, test_case);
      })) {
    return false;
  }
  for (int length = 1; length <= 256; ++length) {
    if (!ExpectReply(
            cli, {"APPEND aof_history x\r\n", std::to_string(length) + "\n"})) {
      return false;
    }
  }
  return true;
}

bool RewriteDataset(cli::RedisCli* const cli) {
  const std::vector<Case> cases = {
      {"BGREWRITEAOF\r\n", "Background append only file rewriting started\n"},
      {"SET aof_string updated\r\n", "OK\n"},
      {"HSET aof_hash second added\r\n", "1\n"},
      {"RPUSH aof_list three\r\n", "3\n"},
      {"SREM aof_set second\r\n", "1\n"},
      {"ZADD aof_zset 2 member\r\n", "0\n"},
  };
  return std::all_of(cases.begin(), cases.end(), [cli](const auto& test_case) {
    return ExpectReply(cli, test_case);
  });
}

bool ReadDataset(cli::RedisCli* const cli) {
  const std::vector<Case> cases = {
      {"GET aof_string\r\n", "updated\n"},
      {"GET aof_expired\r\n", "(nil)\n"},
      {"HGET aof_hash field\r\n", "value\n"},
      {"HGET aof_hash second\r\n", "added\n"},
      {"SISMEMBER aof_set first\r\n", "1\n"},
      {"SISMEMBER aof_set second\r\n", "0\n"},
      {"ZSCORE aof_zset member\r\n", "2\n"},
      {"GET aof_deleted\r\n", "(nil)\n"},
      {"GET aof_history\r\n", std::string(256, 'x') + "\n"},
  };
  if (!std::all_of(cases.begin(), cases.end(), [cli](const auto& test_case) {
        return ExpectReply(cli, test_case);
      })) {
    return false;
  }

  cli->AddCommand("LRANGE aof_list 0 -1\r\n");
  if (NonEmptyLines(cli->ReadReply()) !=
      std::vector<std::string>({"one", "two", "three"})) {
    return false;
  }

  cli->AddCommand("PTTL aof_ttl\r\n");
  const std::string ttl_reply = cli->ReadReply();
  int64_t ttl = 0;
  const auto parsed = std::from_chars(ttl_reply.data(),
                                      ttl_reply.data() + ttl_reply.size(), ttl);
  return parsed.ec == std::errc() && ttl > 0 && ttl <= 60'000;
}
}  // namespace

int Run(std::string_view mode) {
  cli::RedisCli cli;
  if (cli.Connect("127.0.0.1", kAofPort) == cli::CliStatus::kError) {
    return EXIT_FAILURE;
  }
  if (mode == "write") {
    return WriteDataset(&cli) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  if (mode == "read") {
    return ReadDataset(&cli) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  if (mode == "rewrite") {
    return RewriteDataset(&cli) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  return EXIT_FAILURE;
}
}  // namespace redis_simple

int main(int argc, char* argv[]) {
  try {
    return argc == 2 ? redis_simple::Run(argv[1]) : EXIT_FAILURE;
  } catch (...) {
    return EXIT_FAILURE;
  }
}
