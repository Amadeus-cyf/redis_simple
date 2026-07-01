#pragma once

namespace redis_simple {
class Client;
}  // namespace redis_simple

namespace redis_simple::command::key {
void HandleDel(Client* client);
void HandleExists(Client* client);
void HandleType(Client* client);
}  // namespace redis_simple::command::key

namespace redis_simple::command::strings {
void HandleGet(Client* client);
void HandleSet(Client* client);
}  // namespace redis_simple::command::strings

namespace redis_simple::command::lists {
void HandleLPush(Client* client);
void HandleRPush(Client* client);
void HandleLPop(Client* client);
void HandleRPop(Client* client);
void HandleLLen(Client* client);
void HandleLRange(Client* client);
}  // namespace redis_simple::command::lists

namespace redis_simple::command::sets {
void HandleSAdd(Client* client);
void HandleSCard(Client* client);
void HandleSIsMember(Client* client);
void HandleSMembers(Client* client);
void HandleSRem(Client* client);
}  // namespace redis_simple::command::sets

namespace redis_simple::command::zsets {
void HandleZAdd(Client* client);
void HandleZCard(Client* client);
void HandleZRange(Client* client);
void HandleZRank(Client* client);
void HandleZRem(Client* client);
void HandleZScore(Client* client);
}  // namespace redis_simple::command::zsets
