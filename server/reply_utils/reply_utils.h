#pragma once

#include <optional>
#include <string>
#include <vector>

#include "server/reply/reply.h"

namespace redis_simple {
namespace reply_utils {
template <typename T, typename std::string ToString(const T&)>
std::optional<const std::string> EncodeList(const std::vector<T>& list) {
  std::vector<std::string> encode_elements;
  for (const T& element : list) {
    if constexpr (std::is_integral_v<T> == true) {
      encode_elements.push_back(reply::FromInt64(element));
    } else {
      encode_elements.push_back(reply::FromBulkString(ToString(element)));
    }
  }
  try {
    const std::string& reply = reply::FromArray(encode_elements);
    return std::optional<const std::string>(reply);
  } catch (const std::exception& e) {
    printf("catch exception while encoding the list: %s", e.what());
    return std::nullopt;
  }
}
}  // namespace reply_utils
}  // namespace redis_simple
