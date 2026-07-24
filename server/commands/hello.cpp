#include <cstdint>
#include <string>

#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/reply.h"
#include "utils/string_utils.h"

namespace redis_simple::command::connection_commands {
namespace {
std::string EncodeHello(reply::ProtocolVersion protocol) {
  constexpr size_t kFieldCount = 7;
  std::string encoded = reply::FromMapHeader(kFieldCount, protocol);
  reply::AppendBulkString("server", &encoded);
  reply::AppendBulkString("redis_simple", &encoded);
  reply::AppendBulkString("version", &encoded);
  reply::AppendBulkString("0.1.0", &encoded);
  reply::AppendBulkString("proto", &encoded);
  encoded.append(reply::FromInt64(static_cast<int64_t>(protocol)));
  reply::AppendBulkString("id", &encoded);
  encoded.append(reply::FromInt64(0));
  reply::AppendBulkString("mode", &encoded);
  reply::AppendBulkString("standalone", &encoded);
  reply::AppendBulkString("role", &encoded);
  reply::AppendBulkString("master", &encoded);
  reply::AppendBulkString("modules", &encoded);
  encoded.append(reply::FromArrayHeader(0));
  return encoded;
}
}  // namespace

void HandleHello(Client* const client) {
  const auto& args = client->Args();
  if (args.size() > 1) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  auto protocol = client->Protocol();
  if (!args.empty()) {
    int64_t version = 0;
    if (!utils::ToInt64(args[0], &version) || (version != 2 && version != 3)) {
      client->AddReply(
          reply::FromError("NOPROTO unsupported protocol version"));
      return;
    }
    protocol = version == 2 ? reply::ProtocolVersion::kResp2
                            : reply::ProtocolVersion::kResp3;
    client->SetProtocol(protocol);
  }
  client->AddReply(EncodeHello(protocol));
}
}  // namespace redis_simple::command::connection_commands
