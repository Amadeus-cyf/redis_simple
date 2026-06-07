#pragma once

#include <memory>
#include <optional>
#include <string>

#include "memory/quicklist.h"

namespace redis_simple {
namespace list {
class List {
 public:
  static List* Init() { return new List(); }
  bool LPush(const std::string& value);
  bool RPush(const std::string& value);
  std::optional<std::string> RPop();
  std::optional<std::string> LPop();
  size_t Size() const { return quicklist_->Size(); }
  size_t NodeCount() const { return quicklist_->NodeCount(); }

 private:
  List();
  std::unique_ptr<in_memory::QuickList> quicklist_;
};
}  // namespace list
}  // namespace redis_simple
