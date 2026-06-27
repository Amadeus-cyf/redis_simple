#include "server/client_connection/callback.h"

#include "server/client_connection/callback/read_query.h"
#include "server/client_connection/callback/write_reply.h"

namespace redis_simple::client_connection {
connection::ConnectionCallback CreateCallback(CallbackType type) {
  switch (type) {
    case CallbackType::kReadQuery:
      return CreateReadQueryCallback();
    case CallbackType::kWriteReply:
      return CreateWriteReplyCallback();
    default:
      return nullptr;
  }
}
}  // namespace redis_simple::client_connection
