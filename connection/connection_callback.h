#pragma once

#include <functional>

namespace redis_simple {
namespace connection {
class Connection;

enum class ConnectionCallbackType {
  kReadQueryFromClient = 1,
  kWriteReplyToClient = 1 << 1,
};

using ConnectionCallback = std::function<void(Connection*)>;
}  // namespace connection
}  // namespace redis_simple
