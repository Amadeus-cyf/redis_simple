#pragma once

#include "connection/connection_callback.h"

namespace redis_simple::client_connection {
enum class CallbackType {
  kReadQuery = 1,
  kWriteReply = 1 << 1,
};

connection::ConnectionCallback CreateCallback(CallbackType type);
}  // namespace redis_simple::client_connection
