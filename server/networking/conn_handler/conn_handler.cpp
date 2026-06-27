#include "server/networking/conn_handler/conn_handler.h"

#include "server/networking/conn_handler/read_client.h"
#include "server/networking/conn_handler/write_client.h"

namespace redis_simple::networking {
std::unique_ptr<connection::ConnHandler> CreateConnHandler(
    const connection::ConnectionHandlerType flag) {
  switch (flag) {
    case connection::ConnectionHandlerType::kReadQueryFromClient:
      return NewReadFromClientHandler();
    case connection::ConnectionHandlerType::kWriteReplyToClient:
      return NewWriteToClientHandler();
    default:
      return nullptr;
  }
}
}  // namespace redis_simple::networking
