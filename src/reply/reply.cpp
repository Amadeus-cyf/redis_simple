#include "reply.h"

namespace redis_simple {
namespace reply {
const std::string& CRLF = "\r\n";
std::string fromString(const std::string& s) {
  std::string reply;
  reply.push_back('+');
  reply.append(s).append(CRLF);
  return reply;
}

std::string fromBulkString(const std::string& s) {
  std::string reply;
  reply.push_back('$');
  reply.append(std::to_string(s.size())).append(CRLF).append(s).append(CRLF);
  return reply;
}

std::string fromInt64(const int64_t i64) {
  std::string reply;
  reply.push_back(':');
  reply.append(std::to_string(i64)).append(CRLF);
  return reply;
}
}  // namespace reply
}  // namespace redis_simple
