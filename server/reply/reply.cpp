#include "reply.h"

#include <sstream>

namespace redis_simple {
namespace reply {
static const std::string& CRLF = "\r\n";
static constexpr const char stringPrefix = '+';
static constexpr const char bulkStringPrefix = '$';
static constexpr const char int64Prefix = ':';
static constexpr const char arrayPrefix = '*';
static constexpr const char doublePrefix = ',';
static constexpr const char nullPrefix = '_';

namespace {
std::string FloatToString(double fl) {
  std::stringstream ss;
  if (fl >= 0.0001 && fl < 100000) {
    ss << std::fixed << fl;
  } else {
    ss << std::scientific << fl;
  }
  return ss.str();
}
}  // namespace

/*
 * Encode a simple string. The string could not contain \r or \n.
 */
std::string FromString(const std::string& s) {
  std::string reply;
  reply.push_back(stringPrefix);
  reply.append(s).append(CRLF);
  return reply;
}

/*
 * Encode any single binary string.
 */
std::string FromBulkString(const std::string& s) {
  std::string reply;
  reply.push_back(bulkStringPrefix);
  reply.append(std::to_string(s.size())).append(CRLF).append(s).append(CRLF);
  return reply;
}

/*
 * Encode integer.
 */
std::string FromInt64(const int64_t i64) {
  std::string reply;
  reply.push_back(int64Prefix);
  reply.append(std::to_string(i64)).append(CRLF);
  return reply;
}

/*
 * Encode list of strings. The function assumes that each element in
 * the input list has already been encoded.
 */
std::string FromArray(const std::vector<std::string>& array) {
  std::string reply;
  reply.push_back(arrayPrefix);
  reply.append(std::to_string(array.size())).append(CRLF);
  for (const std::string& str : array) {
    if (str.size() < 2 || str.substr(str.size() - 2) != CRLF) {
      throw std::invalid_argument("array element not encoded");
    }
    reply.append(str);
  }
  return reply;
}

/*
 * Encode float
 */
std::string FromFloat(const double fl) {
  std::string reply;
  reply.push_back(doublePrefix);
  reply.append(FloatToString(fl)).append(CRLF);
  return reply;
}

/*
 * Encode null
 */
std::string Null() { return "_\r\n"; }
}  // namespace reply
}  // namespace redis_simple
