#pragma once

#include "connection/connection_callback.h"

namespace redis_simple::client_connection {
connection::ConnectionCallback CreateReadQueryFromClientCallback();
}  // namespace redis_simple::client_connection
