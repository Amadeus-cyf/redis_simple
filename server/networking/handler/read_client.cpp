#include "read_client.h"

#include "server/client.h"

namespace redis_simple {
namespace networking {
namespace {
class ReadFromClientHandler : public connection::ConnHandler {
 public:
  void handle(connection::Connection* conn) override;

 private:
  void readQueryFromClient(connection::Connection* conn);
};

void ReadFromClientHandler::handle(connection::Connection* conn) {
  readQueryFromClient(conn);
}

void ReadFromClientHandler::readQueryFromClient(connection::Connection* conn) {
  printf("read query from client\n");
  Client* c = static_cast<Client*>(conn->getPrivateData());
  if (!c) {
    printf("invalid client\n");
    return;
  }
  ssize_t nread = c->readQuery();
  if (nread <= 0) {
    if (conn->getState() != connection::ConnState::connStateConnected) {
      c->free();
    }
    return;
  }
  if (c->processInputBuffer() == ClientStatus::clientErr) {
    printf("process query buffer failed\n");
  }
  if (c->hasPendingReplies() && !c->getConn()->hasWriteHandler()) {
    printf("client has pending replies, install write handler\n");
    c->getConn()->setWriteHandler(connection::ConnHandler::create(
        connection::ConnHandlerType::writeReplyToClient));
  }
}
}  // namespace

std::unique_ptr<connection::ConnHandler> NewReadFromClientHandler() {
  return std::make_unique<ReadFromClientHandler>();
}
}  // namespace networking
}  // namespace redis_simple
