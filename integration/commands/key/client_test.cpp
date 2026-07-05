#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include "cli/cli.h"
#include "utils/string_utils.h"

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

bool ExpectIntegerBetween(cli::RedisCli* cli, const std::string& command,
                          int64_t min_value, int64_t max_value) {
  cli->AddCommand(command);
  const std::string reply = cli->ReadReply();
  int64_t value = 0;
  if (reply.empty() ||
      !utils::ToInt64(reply.substr(0, reply.size() - 1), &value) ||
      value < min_value || value > max_value) {
    RS_LOG_DEBUG("command failed: %s expected range: [%lld, %lld] actual: %s\n",
                 command.c_str(), min_value, max_value, reply.c_str());
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
      {"HSET key_hash field value\r\n", "1\n"},
      {"TYPE key_string\r\n", "string\n"},
      {"TYPE key_set\r\n", "set\n"},
      {"TYPE key_list\r\n", "list\n"},
      {"TYPE key_zset\r\n", "zset\n"},
      {"TYPE key_hash\r\n", "hash\n"},
      {"EXISTS key_string key_set key_list key_zset key_hash missing_key\r\n",
       "5\n"},
      {"DEL key_string missing_key\r\n", "1\n"},
      {"TYPE key_string\r\n", "none\n"},
      {"EXISTS key_string key_set\r\n", "1\n"},
      {"DEL key_set key_list key_zset key_hash\r\n", "4\n"},
      {"EXISTS key_set key_list key_zset key_hash\r\n", "0\n"},
      {"TYPE key_set\r\n", "none\n"},
      {"SET expire_key value\r\n", "1\n"},
      {"EXPIRE expire_key 10\r\n", "1\n"},
      {"PERSIST expire_key\r\n", "1\n"},
      {"TTL expire_key\r\n", "-1\n"},
      {"PEXPIRE expire_key 5\r\n", "1\n"},
      {"PERSIST missing_expire_key\r\n", "0\n"},
      {"EXPIRE missing_expire_key 10\r\n", "0\n"},
      {"PTTL missing_expire_key\r\n", "-2\n"},
      {"SET rename_source value\r\n", "1\n"},
      {"RENAME rename_source rename_dest\r\n", "OK\n"},
      {"GET rename_dest\r\n", "value\n"},
      {"UNLINK rename_dest missing_key\r\n", "1\n"},
  };

  for (const Case& test_case : cases) {
    if (!ExpectReply(&cli, test_case)) {
      return EXIT_FAILURE;
    }
  }

  if (!ExpectReply(&cli, {"SET ttl_key value PX 5000\r\n", "1\n"})) {
    return EXIT_FAILURE;
  }
  if (!ExpectIntegerBetween(&cli, "TTL ttl_key\r\n", 1, 5)) {
    return EXIT_FAILURE;
  }
  if (!ExpectIntegerBetween(&cli, "PTTL ttl_key\r\n", 1, 5000)) {
    return EXIT_FAILURE;
  }
  if (!ExpectReply(&cli, {"SET ttl_key kept KEEPTTL\r\n", "1\n"})) {
    return EXIT_FAILURE;
  }
  if (!ExpectIntegerBetween(&cli, "PTTL ttl_key\r\n", 1, 5000)) {
    return EXIT_FAILURE;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  if (!ExpectReply(&cli, {"GET expire_key\r\n", "(nil)\n"})) {
    return EXIT_FAILURE;
  }
  if (!ExpectReply(&cli, {"DBSIZE\r\n", "1\n"})) {
    return EXIT_FAILURE;
  }
  if (!ExpectReply(&cli, {"FLUSHDB\r\n", "OK\n"})) {
    return EXIT_FAILURE;
  }
  if (!ExpectReply(&cli, {"DBSIZE\r\n", "0\n"})) {
    return EXIT_FAILURE;
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
