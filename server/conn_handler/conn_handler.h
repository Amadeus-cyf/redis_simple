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
  static std::unique_ptr<ConnHandler> create(const ConnHandlerType flag);
  virtual void handle(Connection* conn) = 0;
  virtual ~ConnHandler() = default;
};
}  // namespace connection
}  // namespace redis_simple
