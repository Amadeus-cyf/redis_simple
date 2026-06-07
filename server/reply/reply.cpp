#include "reply.h"

#include <sstream>

#include "utils/float_utils.h"

namespace redis_simple {
namespace reply {
static const std::string& kCrlf = "\r\n";
static constexpr const char kStringPrefix = '+';
static constexpr const char kBulkStringPrefix = '$';
static constexpr const char kInt64Prefix = ':';
static constexpr const char kArrayPrefix = '*';
static constexpr const char kDoublePrefix = ',';
static constexpr const char kNullPrefix = '_';

/*
 * Encode a simple string. The string could not contain \r or \n.
 */
std::string FromString(const std::string& s) {
  std::string reply;
  reply.push_back(kStringPrefix);
  reply.append(s).append(kCrlf);
  return reply;
}

/*
 * Encode any single binary string.
 */
std::string FromBulkString(const std::string& s) {
  std::string reply;
  reply.push_back(kBulkStringPrefix);
  reply.append(std::to_string(s.size())).append(kCrlf).append(s).append(kCrlf);
  return reply;
}

/*
 * Encode integer.
 */
std::string FromInt64(const int64_t i64) {
  std::string reply;
  reply.push_back(kInt64Prefix);
  reply.append(std::to_string(i64)).append(kCrlf);
  return reply;
}

/*
 * Encode list of strings. The function assumes that each element in
 * the input list has already been encoded.
 */
std::string FromArray(const std::vector<std::string>& array) {
  std::string reply;
  reply.push_back(kArrayPrefix);
  reply.append(std::to_string(array.size())).append(kCrlf);
  for (const std::string& str : array) {
    if (str.size() < kCrlf.size() ||
        str.compare(str.size() - kCrlf.size(), kCrlf.size(), kCrlf) != 0) {
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
  reply.push_back(kDoublePrefix);
  reply.append(utils::FloatToString(fl)).append(kCrlf);
  return reply;
}

/*
 * Encode null
 */
std::string Null() { return "_\r\n"; }
}  // namespace reply
}  // namespace redis_simple
