#include "server/client_connection/callback/write_reply.h"

#include <any>

#include "server/client.h"
#include "server/server.h"

namespace redis_simple::client_connection {
namespace {
ssize_t WriteToClient(Client* client) {
  ssize_t nwritten = 0;
  ssize_t bytes_written = 0;
  while (client->HasPendingReplies()) {
    bytes_written = client->SendReply();
    if (bytes_written <= 0) {
      break;
    }
    nwritten += bytes_written;
  }
  if (bytes_written == -1) {
    if (client->Connection()->State() !=
        connection::ConnectionState::kConnected) {
      client->Free();
      Server::Get()->RemoveClient(client);
      return -1;
    }
  }
  if (!client->HasPendingReplies() &&
      !client->Connection()->UnsetWriteCallback()) {
    // Ignore the failure of uninstalling the write callback.
    RS_LOG_DEBUG("failed to unset the write callback\n");
  }
  return nwritten;
}

void SendReplyToClient(connection::Connection* conn) {
  auto* client = std::any_cast<Client*>(conn->PrivateData());
  RS_LOG_DEBUG("write reply called %d\n", client->HasPendingReplies());
  WriteToClient(client);
}
}  // namespace

connection::ConnectionCallback CreateWriteReplyCallback() {
  return SendReplyToClient;
}
}  // namespace redis_simple::client_connection
