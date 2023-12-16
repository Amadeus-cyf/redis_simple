#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace redis_simple {
class Client;

namespace command {
class Command {
 public:
  static std::weak_ptr<const Command> create(const std::string& name);
  Command(const std::string& name) : name(name){};
  virtual void exec(Client* const client) const = 0;
  const std::string& getName() const { return name; }
  virtual ~Command() = default;

 private:
  static const std::unordered_map<std::string, std::shared_ptr<const Command>>&
      cmdmap;
  const std::string& name;
};
}  // namespace command
}  // namespace redis_simple
