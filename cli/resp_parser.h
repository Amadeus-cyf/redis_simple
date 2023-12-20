#include <string>

namespace redis_simple {
namespace cli {
namespace resp_parser {
ssize_t Parse(const std::string& resp, std::string& reply);
}
}  // namespace cli
}  // namespace redis_simple
