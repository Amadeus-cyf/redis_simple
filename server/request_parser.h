#pragma once

#include <cstddef>
#include <string_view>

#include "server/commands/command.h"

namespace redis_simple::request_parser {
enum class ParseStatus {
  kComplete,
  kIncomplete,
  kInvalid,
};

struct ParseResult {
  ParseStatus status;
  size_t consumed;
};

// Parses one inline or RESP array request. Returned views borrow from input.
ParseResult Parse(std::string_view input, std::string_view* command,
                  command::CommandArgs* args);
}  // namespace redis_simple::request_parser
