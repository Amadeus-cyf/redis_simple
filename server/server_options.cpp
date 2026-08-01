#include "server/server_options.h"

#include <charconv>
#include <string_view>
#include <system_error>

namespace redis_simple {
namespace {
constexpr int kMinPort = 1;
constexpr int kMaxPort = 65535;

bool ParsePort(std::string_view value, int* const port) {
  if (value.empty()) {
    return false;
  }
  int parsed = 0;
  const auto result =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (result.ec != std::errc() || result.ptr != value.data() + value.size() ||
      parsed < kMinPort || parsed > kMaxPort) {
    return false;
  }
  *port = parsed;
  return true;
}
}  // namespace

OptionsResult ParseServerOptions(int argc, const char* const* argv) {
  OptionsResult result;
  for (int index = 1; index < argc; ++index) {
    const std::string_view option(argv[index]);
    if (option == "--help" || option == "-h") {
      result.status = OptionsStatus::kHelp;
      return result;
    }
    if (option != "--bind" && option != "--port") {
      result.status = OptionsStatus::kError;
      result.error = "unknown option";
      return result;
    }
    if (++index >= argc) {
      result.status = OptionsStatus::kError;
      result.error = "missing option value";
      return result;
    }

    const std::string_view value(argv[index]);
    if (option == "--bind") {
      if (value.empty()) {
        result.status = OptionsStatus::kError;
        result.error = "bind address cannot be empty";
        return result;
      }
      result.options.bind_address.assign(value.data(), value.size());
      continue;
    }
    if (!ParsePort(value, &result.options.port)) {
      result.status = OptionsStatus::kError;
      result.error = "port must be between 1 and 65535";
      return result;
    }
  }
  return result;
}

std::string_view ServerUsage() {
  return "Usage: redis_simple [--bind <address>] [--port <port>]\n";
}
}  // namespace redis_simple
