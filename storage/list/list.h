#pragma once

#include <memory>
#include <optional>
#include <string>

#include "memory/listpack.h"

namespace redis_simple {
namespace list {
class List {
 public:
  static List* Init() { return new List(); }
  bool LPush(const std::string& value);
  bool RPush(const std::string& value);
  std::optional<std::string> RPop();
  std::optional<std::string> LPop();
  size_t Size() { return listpack_->Size(); }

 private:
  List();
  std::unique_ptr<in_memory::ListPack> listpack_;
};
}  // namespace list
}  // namespace redis_simple
