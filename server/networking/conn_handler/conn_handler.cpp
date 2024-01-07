#include "server/networking/conn_handler/conn_handler.h"

#include "server/networking/conn_handler/read_client.h"
#include "server/networking/conn_handler/write_client.h"

namespace redis_simple {
namespace networking {
std::unique_ptr<connection::ConnHandler> CreateConnHandler(
    const connection::ConnHandlerType flag) {
  switch (flag) {
    case connection::ConnHandlerType::readQueryFromClient:
      return NewReadFromClientHandler();
    case connection::ConnHandlerType::writeReplyToClient:
      return NewWriteToClientHandler();
    default:
      return nullptr;
  }
}
}  // namespace networking
}  // namespace redis_simple
