#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "memory/listpack.h"
#include "memory/quicklist.h"

namespace redis_simple::list {
class List {
 public:
  enum class Encoding {
    kListPack,
    kQuickList,
  };

  static constexpr size_t kDefaultListMaxListpackBytes =
      in_memory::QuickList::kDefaultNodeMaxBytes;

  static List* Init(
      size_t list_max_listpack_bytes = kDefaultListMaxListpackBytes) {
    return new List(list_max_listpack_bytes);
  }

  bool LPush(const std::string& value);
  bool RPush(const std::string& value);
  std::optional<std::string> RPop();
  std::optional<std::string> LPop();
  size_t Size() const;
  size_t NodeCount() const;
  std::vector<std::string> Range(size_t start, size_t stop) const;
  Encoding GetEncoding() const;

 private:
  explicit List(size_t list_max_listpack_bytes);
  bool Push(const std::string& value, bool head);
  std::optional<std::string> Pop(bool head);
  bool WouldExceedListpackLimit(const std::string& value) const;
  bool ConvertListPackToQuickList();
  void TryConvertQuickListToListPack();

  std::unique_ptr<in_memory::ListPack> listpack_;
  std::unique_ptr<in_memory::QuickList> quicklist_;
  size_t list_max_listpack_bytes_;
};
}  // namespace redis_simple::list
