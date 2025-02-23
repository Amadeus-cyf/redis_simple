#include "storage/list/list.h"

namespace redis_simple {
namespace list {
List::List() : listpack_(std::make_unique<in_memory::ListPack>()) {}

bool List::LPush(const std::string& value) { return listpack_->Prepend(value); }

bool List::RPush(const std::string& value) { return listpack_->Append(value); }

std::optional<std::string> List::RPop() {
  if (listpack_->Size() == 0) return std::nullopt;
  size_t idx = listpack_->Last();
  const std::optional<std::string>& val_opt = listpack_->Get(idx);
  listpack_->Delete(idx);
  return val_opt;
}

std::optional<std::string> List::LPop() {
  if (listpack_->Size() == 0) return std::nullopt;
  size_t idx = listpack_->First();
  const std::optional<std::string>& val_opt = listpack_->Get(idx);
  listpack_->Delete(idx);
  return val_opt;
}
}  // namespace list
}  // namespace redis_simple
