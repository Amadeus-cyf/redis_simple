#pragma once

#include <memory>

namespace redis_simple {
namespace connection {
class Connection;

enum class ConnectionHandlerType {
  kReadQueryFromClient = 1,
  kWriteReplyToClient = 1 << 1,
};

class ConnHandler {
 public:
  virtual void Handle(Connection* conn) = 0;
  virtual ~ConnHandler() = default;
};
}  // namespace connection
}  // namespace redis_simple
