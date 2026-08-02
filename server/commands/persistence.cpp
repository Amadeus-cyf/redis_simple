#include "server/aof.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/reply.h"

namespace redis_simple::command::persistence {
void HandleBgRewriteAof(Client* const client) {
  auto* const aof = client->Aof();
  if (aof == nullptr) {
    client->AddReply(reply::FromError("ERR append only file is disabled"));
    return;
  }

  switch (aof->StartRewrite(client->Db())) {
    case aof::RewriteResult::kStarted:
      client->AddReply(
          reply::FromString("Background append only file rewriting started"));
      return;
    case aof::RewriteResult::kInProgress:
      client->AddReply(reply::FromError(
          "ERR background append only file rewrite already in progress"));
      return;
    case aof::RewriteResult::kError:
      client->AddReply(
          reply::FromError("ERR background append only file rewrite failed"));
      return;
  }
}
}  // namespace redis_simple::command::persistence
