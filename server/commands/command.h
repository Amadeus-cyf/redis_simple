#pragma once

#include <string>

namespace redis_simple {
class Client;
}  // namespace redis_simple

namespace redis_simple::command {
using CommandCallback = void (*)(Client* client);

struct Command {
  const char* name;
  CommandCallback callback;
};

const Command* Find(const std::string& name);
}  // namespace redis_simple::command
