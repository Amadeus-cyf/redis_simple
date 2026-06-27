#include <iomanip>
#include <limits>
#include <sstream>

namespace redis_simple::utils {
std::string FloatToString(double fl) {
  std::stringstream ss;
  ss << std::setprecision(std::numeric_limits<double>::digits10);
  if (fl >= 0.0001 && fl < 100000) {
    ss << std::fixed << fl;
  } else {
    ss << std::scientific << fl;
  }
  return ss.str();
}
}  // namespace redis_simple::utils
