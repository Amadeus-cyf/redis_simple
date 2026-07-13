#pragma once

#include <string_view>
#include <vector>

namespace redis_simple {
class Client;
}  // namespace redis_simple

namespace redis_simple::command {
using CommandArgs = std::vector<std::string_view>;
using CommandCallback = void (*)(Client* client);

struct Command {
  const char* name;
  CommandCallback callback;
};

const Command* Find(std::string_view name);
}  // namespace redis_simple::command
