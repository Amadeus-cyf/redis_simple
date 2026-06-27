#include "server/networking/connection_callback/connection_callback.h"

#include "server/networking/connection_callback/read_client_callback.h"
#include "server/networking/connection_callback/write_client_callback.h"

namespace redis_simple::networking {
connection::ConnectionCallback CreateConnectionCallback(
    connection::ConnectionCallbackType type) {
  switch (type) {
    case connection::ConnectionCallbackType::kReadQueryFromClient:
      return CreateReadFromClientCallback();
    case connection::ConnectionCallbackType::kWriteReplyToClient:
      return CreateWriteToClientCallback();
    default:
      return nullptr;
  }
}
}  // namespace redis_simple::networking
