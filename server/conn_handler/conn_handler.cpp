#include "conn_handler.h"

#include "server/networking/handler/read_client.h"
#include "server/networking/handler/write_client.h"

namespace redis_simple {
namespace connection {
std::unique_ptr<ConnHandler> ConnHandler::Create(const ConnHandlerType flag) {
  switch (flag) {
    case ConnHandlerType::readQueryFromClient:
      return networking::NewReadFromClientHandler();
    case ConnHandlerType::writeReplyToClient:
      return networking::NewWriteToClientHandler();
    default:
      return nullptr;
  }
}
}  // namespace connection
}  // namespace redis_simple
