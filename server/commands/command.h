#pragma once

#include <string>
#include <unordered_map>

namespace redis_simple {
class Client;
}  // namespace redis_simple

namespace redis_simple::command {
using CommandCallback = void (*)(Client* client);

struct Command {
  std::string name;
  CommandCallback callback;
};

const Command* Find(const std::string& name);
}  // namespace redis_simple::command
