#include "read_client_callback.h"

#include <any>

#include "server/client.h"
#include "server/networking/connection_callback/connection_callback.h"
#include "server/networking/networking.h"
#include "server/server.h"

namespace redis_simple::networking {
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
    client->Connection()->SetWriteCallback(networking::CreateConnectionCallback(
        connection::ConnectionCallbackType::kWriteReplyToClient));
  }
}
}  // namespace

connection::ConnectionCallback CreateReadFromClientCallback() {
  return ReadQueryFromClient;
}
}  // namespace redis_simple::networking
