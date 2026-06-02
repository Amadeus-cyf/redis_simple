#pragma once

#include <optional>
#include <string>
#include <vector>

#include "logging/logger.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace reply_utils {
template <typename T, typename std::string ToString(const T&)>
std::optional<std::string> EncodeList(const std::vector<T>& list) {
  std::vector<std::string> encode_elements;
  for (const T& element : list) {
    if constexpr (std::is_integral_v<T>) {
      encode_elements.push_back(reply::FromInt64(element));
    } else {
      encode_elements.push_back(reply::FromBulkString(ToString(element)));
    }
  }
  try {
    return reply::FromArray(encode_elements);
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception while encoding the list: %s", e.what());
    return std::nullopt;
  }
}
}  // namespace reply_utils
}  // namespace redis_simple
