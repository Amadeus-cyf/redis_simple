#include "server/client_connection/callback/read_query.h"

#include <any>

#include "logging/logger.h"
#include "server/client.h"
#include "server/client_connection/callback.h"
#include "server/client_connection/client_connection.h"
#include "server/server.h"

namespace redis_simple::client_connection {
namespace {
void ReadQuery(connection::Connection* conn) {
  RS_LOG_DEBUG("read query from client\n");
  const auto* client_data = std::any_cast<Client*>(&conn->PrivateData());
  if (client_data == nullptr || *client_data == nullptr) {
    RS_LOG_DEBUG("invalid client\n");
    return;
  }
  Client* client = *client_data;
  ssize_t nread = client->ReadQuery();
  if (nread <= 0) {
    if (conn->State() != connection::ConnectionState::kConnected) {
      RS_LOG_DEBUG("client free\n");
      Server::Get()->RemoveClient(client);
    }
    return;
  }
  if (client->ProcessInputBuffer() == ClientStatus::kError) {
    RS_LOG_DEBUG("process query buffer failed\n");
  }
  if (conn->State() != connection::ConnectionState::kConnected) {
    Server::Get()->RemoveClient(client);
    return;
  }
  if (client->HasPendingReplies() &&
      !client->Connection()->HasWriteCallback()) {
    RS_LOG_DEBUG("client has pending replies, install write callback\n");
    client->Connection()->SetWriteCallback(
        CreateCallback(CallbackType::kWriteReply));
  }
}
}  // namespace

connection::ConnectionCallback CreateReadQueryCallback() { return ReadQuery; }
}  // namespace redis_simple::client_connection
