#pragma once

#include <string>
#include <unordered_map>

namespace redis_simple {
class Client;

namespace command {
using CommandCallback = void (*)(Client* client);

struct Command {
  std::string name;
  CommandCallback callback;
};

const Command* Find(const std::string& name);
}  // namespace command
}  // namespace redis_simple
