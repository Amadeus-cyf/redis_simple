#pragma once

#include "connection/connection_callback.h"

namespace redis_simple {
namespace networking {
connection::ConnectionCallback CreateReadFromClientCallback();
}
}  // namespace redis_simple
