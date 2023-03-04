#pragma once

#include "server/conn_handler/conn_handler.h"

namespace redis_simple {
namespace networking {
std::unique_ptr<connection::ConnHandler> NewReadFromClientHandler();
}
}  // namespace redis_simple
