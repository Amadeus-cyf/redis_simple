#include "write_client.h"

#include <any>

#include "server/client.h"

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
  Client* c = std::any_cast<Client*>(conn->PrivateData());
  printf("write reply called %d\n", c->HasPendingReplies());
  WriteToClient(c);
}

ssize_t WriteToClientHandler::WriteToClient(Client* c) {
  ssize_t nwritten = 0, r = 0;
  while (c->HasPendingReplies()) {
    r = c->SendReply();
    if (r <= 0) break;
    nwritten += r;
  }
  if (r == -1) {
    if (c->Connection()->State() != connection::ConnState::connStateConnected) {
      c->Free();
      return -1;
    }
  }
  if (!c->HasPendingReplies() && !c->Connection()->UnsetWriteHandler()) {
    /* ignore the failure of uninstalling the write handler */
    printf("failed to unset the write handler\n");
  }
  return nwritten;
}
}  // namespace

std::unique_ptr<connection::ConnHandler> NewWriteToClientHandler() {
  return std::make_unique<WriteToClientHandler>();
}
}  // namespace networking
}  // namespace redis_simple
