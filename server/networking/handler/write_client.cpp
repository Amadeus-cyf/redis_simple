#include "write_client.h"

#include "server/client.h"
#include "server/connection/connection.h"

namespace redis_simple {
namespace networking {
namespace {
class WriteToClientHandler : public connection::ConnHandler {
 public:
  void handle(connection::Connection* conn) override;

 private:
  void sendReplyToClient(connection::Connection* conn);
  ssize_t writeToClient(Client* c);
};

void WriteToClientHandler::handle(connection::Connection* conn) {
  sendReplyToClient(conn);
}

void WriteToClientHandler::sendReplyToClient(connection::Connection* conn) {
  Client* c = static_cast<Client*>(conn->getPrivateData());
  printf("write reply called %d\n", c->hasPendingReplies());
  while (c->hasPendingReplies() && writeToClient(c) > 0)
    ;
  if (!c->hasPendingReplies()) {
    conn->unsetWriteHandler();
  }
}

ssize_t WriteToClientHandler::writeToClient(Client* c) {
  ssize_t nwritten = 0;
  if (c->getFlags() & ClientType::clientSlave) {
    // TODO: send repl block to slave
  } else {
    nwritten = c->sendReply();
    if (nwritten <= 0) {
      if (c->getConn()->getState() !=
          connection::ConnState::connStateConnected) {
        printf("free client\n");
      }
      return -1;
    }
  }
  return nwritten;
}
}  // namespace

std::unique_ptr<connection::ConnHandler> NewWriteToClientHandler() {
  return std::make_unique<WriteToClientHandler>();
}
}  // namespace networking
}  // namespace redis_simple
