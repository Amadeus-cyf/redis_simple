#pragma once

#include <string>

#include "memory/dynamic_buffer.h"
#include "memory/reply_buffer.h"
#include "server/commands/command.h"
#include "server/connection/connection.h"
#include "server/db/db.h"
#include "server/networking/networking.h"

namespace redis_simple {
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
  connection::Connection* getConn() { return conn.get(); }
  std::weak_ptr<const db::RedisDb> getDb() { return db; }
  ssize_t readQuery();
  ssize_t sendReply();
  size_t addReply(const std::string& s) {
    return buf->addReplyToBufferOrList(s.c_str(), s.length());
  }
  bool hasPendingReplies() { return !(buf->isEmpty()); }
  ClientStatus processInputBuffer();
  void free() { conn->connClose(); }
  void free() const { conn->connClose(); }
  const std::vector<std::string>& getArgs() { return args; }

 private:
  explicit Client(connection::Connection* connection);
  ClientStatus processInlineBuffer();
  ClientStatus processCommand();
  void setCmd(std::weak_ptr<const command::Command> command) { cmd = command; }
  void setArgs(const std::vector<std::string>& _args) { args = _args; }
  ssize_t _sendReply();
  ssize_t _sendvReply();
  std::weak_ptr<const db::RedisDb> db;
  int flags;
  /* current command */
  std::weak_ptr<const command::Command> cmd;
  /* current command args*/
  std::vector<std::string> args;
  /* connection */
  std::unique_ptr<connection::Connection> conn;
  /* in memory buffer used to store incoming query */
  std::unique_ptr<in_memory::DynamicBuffer> query_buf;
  /* in memory buffer used to store reply of command */
  std::unique_ptr<in_memory::ReplyBuffer> buf;
};
}  // namespace redis_simple
