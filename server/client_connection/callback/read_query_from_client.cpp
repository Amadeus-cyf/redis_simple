#include "server/client_connection/callback/read_query_from_client.h"

#include <any>

#include "server/client.h"
#include "server/client_connection/callback.h"
#include "server/client_connection/client_connection.h"
#include "server/server.h"

namespace redis_simple::client_connection {
namespace {
void ReadQueryFromClient(connection::Connection* conn) {
  RS_LOG_DEBUG("read query from client\n");
  auto* client = std::any_cast<Client*>(conn->PrivateData());
  if (client == nullptr) {
    RS_LOG_DEBUG("invalid client\n");
    return;
  }
  ssize_t nread = client->ReadQuery();
  if (nread <= 0) {
    if (conn->State() != connection::ConnectionState::kConnected) {
      RS_LOG_DEBUG("client free\n");
      client->Free();
      Server::Get()->RemoveClient(client);
    }
    return;
  }
  if (client->ProcessInputBuffer() == ClientStatus::kError) {
    RS_LOG_DEBUG("process query buffer failed\n");
  }
  if (client->HasPendingReplies() &&
      !client->Connection()->HasWriteCallback()) {
    RS_LOG_DEBUG("client has pending replies, install write callback\n");
    client->Connection()->SetWriteCallback(
        CreateCallback(CallbackType::kWriteReplyToClient));
  }
}
}  // namespace

connection::ConnectionCallback CreateReadQueryFromClientCallback() {
  return ReadQueryFromClient;
}
}  // namespace redis_simple::client_connection
