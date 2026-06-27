#include "write_client.h"

#include <any>

#include "server/client.h"
#include "server/server.h"

namespace redis_simple::networking {
namespace {
class WriteToClientHandler : public connection::ConnHandler {
 public:
  void Handle(connection::Connection* conn) override;

 private:
  void SendReplyToClient(connection::Connection* conn);
  static ssize_t WriteToClient(Client* c);
};

void WriteToClientHandler::Handle(connection::Connection* conn) {
  SendReplyToClient(conn);
}

void WriteToClientHandler::SendReplyToClient(connection::Connection* conn) {
  auto* c = std::any_cast<Client*>(conn->PrivateData());
  RS_LOG_DEBUG("write reply called %d\n", c->HasPendingReplies());
  WriteToClient(c);
}

ssize_t WriteToClientHandler::WriteToClient(Client* c) {
  ssize_t nwritten = 0;
  ssize_t r = 0;
  while (c->HasPendingReplies()) {
    r = c->SendReply();
    if (r <= 0) {
      break;
    }
    nwritten += r;
  }
  if (r == -1) {
    if (c->Connection()->State() != connection::ConnectionState::kConnected) {
      c->Free();
      Server::Get()->RemoveClient(c);
      return -1;
    }
  }
  if (!c->HasPendingReplies() && !c->Connection()->UnsetWriteHandler()) {
    // Ignore the failure of uninstalling the write handler.
    RS_LOG_DEBUG("failed to unset the write handler\n");
  }
  return nwritten;
}
}  // namespace

std::unique_ptr<connection::ConnHandler> NewWriteToClientHandler() {
  return std::make_unique<WriteToClientHandler>();
}
}  // namespace redis_simple::networking
