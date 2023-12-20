#include "write_client.h"

#include "server/client.h"
#include "server/connection/connection.h"

namespace redis_simple {
namespace networking {
namespace {
class WriteToClientHandler : public connection::ConnHandler {
 public:
  void Handle(connection::Connection* conn) override;

 private:
  void SendReplyToClient(connection::Connection* conn);
  ssize_t WriteToClient(Client* c);
};

void WriteToClientHandler::Handle(connection::Connection* conn) {
  SendReplyToClient(conn);
}

void WriteToClientHandler::SendReplyToClient(connection::Connection* conn) {
  Client* c = static_cast<Client*>(conn->PrivateData());
  printf("write reply called %d\n", c->hasPendingReplies());
  WriteToClient(c);
}

ssize_t WriteToClientHandler::WriteToClient(Client* c) {
  ssize_t nwritten = 0, r = 0;
  while (c->hasPendingReplies()) {
    r = c->sendReply();
    if (r <= 0) break;
    nwritten += r;
  }
  if (r == -1) {
    if (c->getConn()->State() != connection::ConnState::connStateConnected) {
      c->free();
      return -1;
    }
  }
  if (!c->hasPendingReplies()) {
    c->getConn()->UnsetWriteHandler();
  }
  return nwritten;
}
}  // namespace

std::unique_ptr<connection::ConnHandler> NewWriteToClientHandler() {
  return std::make_unique<WriteToClientHandler>();
}
}  // namespace networking
}  // namespace redis_simple
