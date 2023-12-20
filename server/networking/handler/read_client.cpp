#include "read_client.h"

#include "server/client.h"

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
  Client* c = static_cast<Client*>(conn->PrivateData());
  if (!c) {
    printf("invalid client\n");
    return;
  }
  ssize_t nread = c->readQuery();
  if (nread <= 0) {
    if (nread == 0 ||
        conn->State() != connection::ConnState::connStateConnected) {
      printf("client free\n");
      c->free();
    }
    return;
  }
  if (c->processInputBuffer() == ClientStatus::clientErr) {
    printf("process query buffer failed\n");
  }
  if (c->hasPendingReplies() && !c->getConn()->HasWriteHandler()) {
    printf("client has pending replies, install write handler\n");
    c->getConn()->SetWriteHandler(connection::ConnHandler::Create(
        connection::ConnHandlerType::writeReplyToClient));
  }
}
}  // namespace

std::unique_ptr<connection::ConnHandler> NewReadFromClientHandler() {
  return std::make_unique<ReadFromClientHandler>();
}
}  // namespace networking
}  // namespace redis_simple
