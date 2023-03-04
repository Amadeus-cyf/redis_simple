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
  printf("nread %zd\n", nread);

  if (nread < 0) {
    if (conn->getState() == connection::ConnState::connStateConnected) {
      return;
    } else {
      // TODO: delete client
      printf("free client\n");
    }
  } else if (nread == 0) {
    return;
  }

  if (c->processInputBuffer() == ClientStatus::clientErr) {
    printf("process query buffer failed\n");
  }
}
}  // namespace

std::unique_ptr<connection::ConnHandler> NewReadFromClientHandler() {
  return std::make_unique<ReadFromClientHandler>();
}
}  // namespace networking
}  // namespace redis_simple
