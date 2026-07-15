#include "utils/float_utils.h"

#include <array>
#include <charconv>
#include <cmath>
#include <string>
#include <string_view>
#include <system_error>

namespace redis_simple::utils {
std::string FloatToString(double fl) {
  std::array<char, 64> buffer{};
  auto [end, error] = std::to_chars(buffer.begin(), buffer.end(), fl);
  if (error != std::errc()) {
    return {};
  }

  const std::string_view general(buffer.data(),
                                 static_cast<size_t>(end - buffer.begin()));
  const size_t exponent_position = general.find('e');
  if (exponent_position != std::string_view::npos) {
    int exponent = 0;
    const std::string_view exponent_text =
        general.substr(exponent_position + 1);
    const char* exponent_start = exponent_text.data();
    if (!exponent_text.empty() && exponent_text.front() == '+') {
      ++exponent_start;
    }
    const auto exponent_result = std::from_chars(
        exponent_start, exponent_text.data() + exponent_text.size(), exponent);
    if (exponent_result.ec == std::errc() && exponent >= -4 && exponent <= 16) {
      const auto fixed = std::to_chars(buffer.begin(), buffer.end(), fl,
                                       std::chars_format::fixed);
      if (fixed.ec == std::errc()) {
        end = fixed.ptr;
      }
    }
  }
  return {buffer.data(), end};
}

bool ToDouble(std::string_view value, double* const result) {
  if (value.empty()) {
    return false;
  }
  const char* begin = value.data();
  const char* const end = begin + value.size();
  if (*begin == '+') {
    ++begin;
  }
  double parsed = 0;
  const auto conversion =
      std::from_chars(begin, end, parsed, std::chars_format::general);
  if (begin == end || conversion.ec != std::errc() || conversion.ptr != end ||
      std::isnan(parsed)) {
    return false;
  }
  if (result != nullptr) {
    *result = parsed;
  }
  return true;
}
}  // namespace redis_simple::utils
