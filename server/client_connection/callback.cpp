#include "server/client_connection/callback.h"

#include "server/client_connection/callback/read_query_from_client.h"
#include "server/client_connection/callback/write_reply_to_client.h"

namespace redis_simple::client_connection {
connection::ConnectionCallback CreateCallback(CallbackType type) {
  switch (type) {
    case CallbackType::kReadQueryFromClient:
      return CreateReadQueryFromClientCallback();
    case CallbackType::kWriteReplyToClient:
      return CreateWriteReplyToClientCallback();
    default:
      return nullptr;
  }
}
}  // namespace redis_simple::client_connection
