#include <string>
#include <vector>

namespace redis_simple {
namespace cli {
namespace resp_parser {
/* parse the first valid response and append the result into reply*/
ssize_t Parse(const std::string& resp, std::vector<std::string>& reply);
}  // namespace resp_parser
}  // namespace cli
}  // namespace redis_simple
