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
  void writeToClient(Client* c);
};

void WriteToClientHandler::handle(connection::Connection* conn) {
  sendReplyToClient(conn);
}

void WriteToClientHandler::sendReplyToClient(connection::Connection* conn) {
  Client* c = static_cast<Client*>(conn->getPrivateData());

  printf("write reply called %d\n", c->hasPendingReplies());

  while (c->hasPendingReplies()) {
    writeToClient(c);
  }

  conn->unsetWriteHandler();
}

void WriteToClientHandler::writeToClient(Client* c) {
  if (c->getFlags() & ClientType::clientSlave) {
    // TODO: send repl block to slave
  } else {
    ssize_t nwritten = c->sendReply();
    if (nwritten < 0) {
      if (c->getConn()->getState() ==
          connection::ConnState::connStateConnected) {
        return;
      } else {
        printf("free client\n");
      }
    } else if (nwritten == 0) {
      return;
    }
    printf("nwrite %zd\n", nwritten);
  }
}
}  // namespace

std::unique_ptr<connection::ConnHandler> NewWriteToClientHandler() {
  return std::make_unique<WriteToClientHandler>();
}
}  // namespace networking
}  // namespace redis_simple
