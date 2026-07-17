#include "server/request_parser.h"

#include <charconv>
#include <cstddef>
#include <limits>
#include <string_view>
#include <system_error>

namespace redis_simple::request_parser {
namespace {
constexpr size_t kMaxCommandElements = size_t{1024} * 1024;

ParseResult FindCrlf(std::string_view input, size_t start) {
  const size_t end = input.find("\r\n", start);
  return end == std::string_view::npos
             ? ParseResult{ParseStatus::kIncomplete, 0}
             : ParseResult{ParseStatus::kComplete, end};
}

bool ParseSize(std::string_view value, size_t* const result) {
  if (value.empty()) {
    return false;
  }
  size_t parsed = 0;
  const auto conversion =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (conversion.ec != std::errc() ||
      conversion.ptr != value.data() + value.size()) {
    return false;
  }
  *result = parsed;
  return true;
}

ParseResult ParseBulkString(std::string_view input, size_t start,
                            std::string_view* value) {
  if (start >= input.size()) {
    return {ParseStatus::kIncomplete, 0};
  }
  if (input[start] != '$') {
    return {ParseStatus::kInvalid, 0};
  }
  const auto header = FindCrlf(input, start + 1);
  if (header.status != ParseStatus::kComplete) {
    return header;
  }
  size_t length = 0;
  if (!ParseSize(input.substr(start + 1, header.consumed - start - 1),
                 &length)) {
    return {ParseStatus::kInvalid, 0};
  }
  const size_t content_start = header.consumed + 2;
  if (length > std::numeric_limits<size_t>::max() - content_start - 2) {
    return {ParseStatus::kInvalid, 0};
  }
  const size_t end = content_start + length;
  if (end + 2 > input.size()) {
    return {ParseStatus::kIncomplete, 0};
  }
  if (input.substr(end, 2) != "\r\n") {
    return {ParseStatus::kInvalid, 0};
  }
  *value = input.substr(content_start, length);
  return {ParseStatus::kComplete, end + 2 - start};
}

ParseResult ParseResp(std::string_view input, std::string_view* command,
                      command::CommandArgs* args) {
  const auto header = FindCrlf(input, 1);
  if (header.status != ParseStatus::kComplete) {
    return header;
  }
  size_t element_count = 0;
  if (!ParseSize(input.substr(1, header.consumed - 1), &element_count) ||
      element_count == 0 || element_count > kMaxCommandElements) {
    return {ParseStatus::kInvalid, 0};
  }

  args->reserve(element_count - 1);
  size_t consumed = header.consumed + 2;
  for (size_t index = 0; index < element_count; ++index) {
    std::string_view value;
    const auto element = ParseBulkString(input, consumed, &value);
    if (element.status != ParseStatus::kComplete) {
      return element;
    }
    if (index == 0) {
      *command = value;
    } else {
      args->push_back(value);
    }
    consumed += element.consumed;
  }
  return {ParseStatus::kComplete, consumed};
}

ParseResult ParseInline(std::string_view input, std::string_view* command,
                        command::CommandArgs* args) {
  const size_t newline = input.find('\n');
  if (newline == std::string_view::npos) {
    return {ParseStatus::kIncomplete, 0};
  }
  size_t line_end = newline;
  if (line_end > 0 && input[line_end - 1] == '\r') {
    --line_end;
  }
  const std::string_view line = input.substr(0, line_end);
  size_t position = line.find_first_not_of(' ');
  if (position == std::string_view::npos) {
    return {ParseStatus::kComplete, newline + 1};
  }

  const size_t command_end = line.find(' ', position);
  *command = line.substr(position, command_end == std::string_view::npos
                                       ? std::string_view::npos
                                       : command_end - position);
  position = command_end;
  while (position != std::string_view::npos) {
    const size_t start = line.find_first_not_of(' ', position);
    if (start == std::string_view::npos) {
      break;
    }
    const size_t end = line.find(' ', start);
    args->push_back(line.substr(start, end == std::string_view::npos
                                           ? std::string_view::npos
                                           : end - start));
    position = end;
  }
  return {ParseStatus::kComplete, newline + 1};
}
}  // namespace

ParseResult Parse(std::string_view input, std::string_view* const command,
                  command::CommandArgs* const args) {
  if (command == nullptr || args == nullptr) {
    return {ParseStatus::kInvalid, 0};
  }
  *command = {};
  args->clear();
  if (input.empty()) {
    return {ParseStatus::kIncomplete, 0};
  }
  const auto result = input.front() == '*' ? ParseResp(input, command, args)
                                           : ParseInline(input, command, args);
  if (result.status != ParseStatus::kComplete) {
    *command = {};
    args->clear();
  }
  return result;
}
}  // namespace redis_simple::request_parser
