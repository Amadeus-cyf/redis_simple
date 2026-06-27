#pragma once

#include "connection/connection_callback.h"

namespace redis_simple {
namespace networking {
connection::ConnectionCallback CreateConnectionCallback(
    connection::ConnectionCallbackType type);
}
}  // namespace redis_simple
