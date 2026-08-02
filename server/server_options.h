#pragma once

#include <string>
#include <string_view>

#include "server/aof.h"

namespace redis_simple {
struct ServerOptions {
  std::string bind_address{"127.0.0.1"};
  int port{8080};
  bool append_only{};
  aof::Options aof_options;
};

enum class OptionsStatus {
  kOk,
  kHelp,
  kError,
};

struct OptionsResult {
  ServerOptions options;
  OptionsStatus status{OptionsStatus::kOk};
  std::string_view error;
};

OptionsResult ParseServerOptions(int argc, const char* const* argv);
std::string_view ServerUsage();
}  // namespace redis_simple
