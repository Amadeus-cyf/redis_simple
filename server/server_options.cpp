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

bool ParseAppendOnly(std::string_view value, bool* const enabled) {
  if (value == "yes") {
    *enabled = true;
    return true;
  }
  if (value == "no") {
    *enabled = false;
    return true;
  }
  return false;
}

bool ParseFsyncPolicy(std::string_view value, aof::FsyncPolicy* const policy) {
  if (value == "always") {
    *policy = aof::FsyncPolicy::kAlways;
    return true;
  }
  if (value == "everysec") {
    *policy = aof::FsyncPolicy::kEverySecond;
    return true;
  }
  if (value == "no") {
    *policy = aof::FsyncPolicy::kNo;
    return true;
  }
  return false;
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
    if (option != "--bind" && option != "--port" && option != "--appendonly" &&
        option != "--appendfilename" && option != "--appendfsync") {
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
    if (option == "--port") {
      if (ParsePort(value, &result.options.port)) {
        continue;
      }
      result.status = OptionsStatus::kError;
      result.error = "port must be between 1 and 65535";
      return result;
    }
    if (option == "--appendonly") {
      if (ParseAppendOnly(value, &result.options.append_only)) {
        continue;
      }
      result.status = OptionsStatus::kError;
      result.error = "appendonly must be yes or no";
      return result;
    }
    if (option == "--appendfilename") {
      if (!value.empty()) {
        result.options.aof_options.path.assign(value.data(), value.size());
        continue;
      }
      result.status = OptionsStatus::kError;
      result.error = "appendfilename cannot be empty";
      return result;
    }
    if (!ParseFsyncPolicy(value, &result.options.aof_options.fsync)) {
      result.status = OptionsStatus::kError;
      result.error = "appendfsync must be always, everysec, or no";
      return result;
    }
  }
  return result;
}

std::string_view ServerUsage() {
  return "Usage: redis_simple [--bind <address>] [--port <port>] "
         "[--appendonly <yes|no>] [--appendfilename <path>] "
         "[--appendfsync <always|everysec|no>]\n";
}
}  // namespace redis_simple
