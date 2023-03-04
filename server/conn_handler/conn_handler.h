#pragma once

#include <memory>

namespace redis_simple {
namespace connection {
class Connection;

enum class ConnHandlerType {
  syncWithMaster = 1,
  readSyncBulkPayload = 1 << 1,
  readQueryFromClient = 1 << 2,
  writeReplyToClient = 1 << 3,
};

class ConnHandler {
 public:
  static std::unique_ptr<ConnHandler> create(const ConnHandlerType flag);
  virtual void handle(Connection* conn) = 0;
  virtual ~ConnHandler() = default;
};
}  // namespace connection
}  // namespace redis_simple
