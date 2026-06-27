#pragma once

#include <functional>

namespace redis_simple::connection {
class Connection;

using ConnectionCallback = std::function<void(Connection*)>;
}  // namespace redis_simple::connection
