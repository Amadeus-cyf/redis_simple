#pragma once

#include <memory>

namespace redis_simple {
namespace connection {
class Connection;

enum class ConnHandlerType {
  readQueryFromClient = 1,
  writeReplyToClient = 1 << 1,
};

class ConnHandler {
 public:
  virtual void Handle(Connection* conn) = 0;
  virtual ~ConnHandler() = default;
};
}  // namespace connection
}  // namespace redis_simple
