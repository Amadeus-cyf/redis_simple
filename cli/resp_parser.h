#include <string>
#include <vector>

namespace redis_simple {
namespace cli {
namespace resp_parser {
ssize_t Parse(const std::string& resp, std::vector<std::string>& reply);
}
}  // namespace cli
}  // namespace redis_simple
