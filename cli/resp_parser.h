#pragma once

#include <string>
#include <vector>

namespace redis_simple::cli::resp_parser {
// Parse the first valid response and append the result into reply.
ssize_t Parse(const std::string& resp, std::vector<std::string>& reply);
}  // namespace redis_simple::cli::resp_parser
