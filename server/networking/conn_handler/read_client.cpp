#include "read_client.h"

#include <any>

#include "server/client.h"
#include "server/networking/conn_handler/conn_handler.h"
#include "server/networking/networking.h"
#include "server/server.h"

namespace redis_simple {
namespace networking {
namespace {
class ReadFromClientHandler : public connection::ConnHandler {
 public:
  void Handle(connection::Connection* conn) override;

 private:
  void ReadQueryFromClient(connection::Connection* conn);
};

void ReadFromClientHandler::Handle(connection::Connection* conn) {
  ReadQueryFromClient(conn);
}

void ReadFromClientHandler::ReadQueryFromClient(connection::Connection* conn) {
  printf("read query from client\n");
  Client* c = std::any_cast<Client*>(conn->PrivateData());
  if (!c) {
    printf("invalid client\n");
    return;
  }
  ssize_t nread = c->ReadQuery();
  if (nread <= 0) {
    if (conn->State() != connection::ConnState::connStateConnected) {
      printf("client free\n");
      c->Free();
      Server::Get()->RemoveClient(c);
    }
    return;
  }
  if (c->ProcessInputBuffer() == ClientStatus::clientErr) {
    printf("process query buffer failed\n");
  }
  if (c->HasPendingReplies() && !c->Connection()->HasWriteHandler()) {
    printf("client has pending replies, install write handler\n");
    c->Connection()->SetWriteHandler(networking::CreateConnHandler(
        connection::ConnHandlerType::writeReplyToClient));
  }
}
}  // namespace

std::unique_ptr<connection::ConnHandler> NewReadFromClientHandler() {
  return std::make_unique<ReadFromClientHandler>();
}
}  // namespace networking
}  // namespace redis_simple
