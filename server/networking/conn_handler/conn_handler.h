#pragma once

#include "connection/conn_handler.h"

namespace redis_simple {
namespace networking {
std::unique_ptr<connection::ConnHandler> CreateConnHandler(
    const connection::ConnHandlerType flag);
}
}  // namespace redis_simple
