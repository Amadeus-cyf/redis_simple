#pragma once

#include <string>

#include "memory/dynamic_buffer.h"
#include "memory/reply_buffer.h"
#include "server/connection/connection.h"
#include "server/db/db.h"
#include "server/networking/networking.h"
#include "server/redis_cmd/redis_cmd.h"

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
  static Client* create(connection::Connection* connection) {
    return new Client(connection);
  }
  int getFlags() { return flags; }
  RedisCommand* getCmd() { return cmd.get(); }
  connection::Connection* getConn() { return conn.get(); }
  const db::RedisDb* getDb() { return db; }
  ssize_t readQuery();
  ssize_t sendReply();
  size_t addReply(const std::string& s) {
    return buf->addReplyToBufferOrList(s.c_str(), s.length());
  }
  bool hasPendingReplies() { return !(buf->isEmpty()); }
  ClientStatus processInputBuffer();
  ClientStatus processCommand();
  void free() { conn->connClose(); }
  void free() const { conn->connClose(); }

 private:
  explicit Client(connection::Connection* connection);
  ClientStatus processInlineBuffer();
  void setCmd(RedisCommand* command) { cmd.reset(command); }
  ssize_t _sendReply();
  ssize_t _sendvReply();
  const db::RedisDb* db;
  int flags;
  std::unique_ptr<RedisCommand> cmd;
  std::unique_ptr<connection::Connection> conn;
  /* in memory buffer used to store incoming query */
  std::unique_ptr<in_memory::DynamicBuffer> query_buf;
  /* in memory buffer used to store reply of command */
  std::unique_ptr<in_memory::ReplyBuffer> buf;
};
}  // namespace redis_simple
