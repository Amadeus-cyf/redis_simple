#pragma once

#include <string>

#include "networking/networking.h"
#include "server/connection/connection.h"
#include "server/db/db.h"
#include "server/memory/query_buffer.h"
#include "server/memory/reply_buffer.h"

namespace redis_simple {
enum ClientType {
  clientSlave = 1,
};

enum class ClientStatus {
  clientOK = 0,
  clientErr = -1,
};

class Client {
 public:
  Client();
  explicit Client(connection::Connection* connection);
  int getFlags() { return flags; }
  RedisCommand* getCmd() { return cmd.get(); }
  void setCmd(RedisCommand* command) { cmd.reset(command); }
  connection::Connection* getConn() { return conn.get(); }
  db::RedisDb* getDb() { return db.get(); }

  ssize_t readQuery();
  ssize_t sendReply();
  size_t addReply(const std::string& s) {
    return buf->addReplyToBufferOrList(s.c_str(), s.length());
  }
  bool hasPendingReplies() { return !(buf->isEmpty()); }

  ClientStatus processInputBuffer();
  ClientStatus processInlineBuffer();

  ClientStatus processCommand();

 private:
  ssize_t _sendReply();
  ssize_t _sendvReply();
  std::shared_ptr<db::RedisDb> db;
  int flags;
  std::unique_ptr<RedisCommand> cmd;
  std::unique_ptr<connection::Connection> conn;
  /* in memory buffer used to store incoming query */
  std::unique_ptr<in_memory::QueryBuffer> query_buf;
  /* in memory buffer used to store reply of command */
  std::unique_ptr<in_memory::ReplyBuffer> buf;
};
}  // namespace redis_simple
