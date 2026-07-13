#pragma once

#include <string>
#include <string_view>

namespace redis_simple::utils {
// Convert floating number to string.
std::string FloatToString(double fl);
// Parse the entire string as a double. NaN and out-of-range values are invalid.
bool ToDouble(std::string_view value, double* result);
}  // namespace redis_simple::utils
