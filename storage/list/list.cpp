#include "storage/list/list.h"

#include <vector>

namespace redis_simple::list {
namespace {
constexpr size_t kEntryOverheadEstimate = 8;
}  // namespace

List::List(size_t list_max_listpack_bytes)
    : listpack_(std::make_unique<in_memory::ListPack>()),
      quicklist_(nullptr),
      list_max_listpack_bytes_(list_max_listpack_bytes) {}

bool List::LPush(const std::string& value) { return Push(value, true); }

bool List::RPush(const std::string& value) { return Push(value, false); }

std::optional<std::string> List::RPop() { return Pop(false); }

std::optional<std::string> List::LPop() { return Pop(true); }

size_t List::Size() const {
  return listpack_ ? listpack_->Size() : quicklist_->Size();
}

size_t List::NodeCount() const {
  return quicklist_ ? quicklist_->NodeCount() : 0;
}

List::Encoding List::GetEncoding() const {
  return listpack_ ? Encoding::kListPack : Encoding::kQuickList;
}

bool List::Push(const std::string& value, bool head) {
  if (listpack_) {
    if (WouldExceedListpackLimit(value) && !ConvertListPackToQuickList()) {
      return false;
    }
    if (listpack_) {
      return head ? listpack_->Prepend(value) : listpack_->Append(value);
    }
  }
  return head ? quicklist_->LPush(value) : quicklist_->RPush(value);
}

std::optional<std::string> List::Pop(bool head) {
  if (listpack_) {
    if (listpack_->Size() == 0) {
      return std::nullopt;
    }
    const size_t idx = head ? listpack_->First() : listpack_->Last();
    auto value = listpack_->Get(idx);
    listpack_->Delete(idx);
    return value;
  }

  auto value = head ? quicklist_->LPop() : quicklist_->RPop();
  TryConvertQuickListToListPack();
  return value;
}

bool List::WouldExceedListpackLimit(const std::string& value) const {
  if (!listpack_) {
    return false;
  }
  return listpack_->GetTotalBytes() + value.size() + kEntryOverheadEstimate >
         list_max_listpack_bytes_;
}

bool List::ConvertListPackToQuickList() {
  auto quicklist =
      std::make_unique<in_memory::QuickList>(list_max_listpack_bytes_);
  ssize_t idx = listpack_->First();
  while (idx != -1) {
    const auto value = listpack_->Get(idx);
    if (value.has_value() && !quicklist->RPush(*value)) {
      return false;
    }
    idx = listpack_->Next(idx);
  }
  quicklist_ = std::move(quicklist);
  listpack_.reset();
  return true;
}

void List::TryConvertQuickListToListPack() {
  if (!quicklist_ || quicklist_->NodeCount() > 1) {
    return;
  }

  std::vector<std::string> values;
  while (auto value = quicklist_->LPop()) {
    values.push_back(*value);
  }

  auto listpack = std::make_unique<in_memory::ListPack>();
  for (const auto& value : values) {
    if (!listpack->Append(value)) {
      auto quicklist =
          std::make_unique<in_memory::QuickList>(list_max_listpack_bytes_);
      for (const auto& rebuild_value : values) {
        quicklist->RPush(rebuild_value);
      }
      quicklist_ = std::move(quicklist);
      return;
    }
  }
  if (listpack->GetTotalBytes() > list_max_listpack_bytes_ / 2) {
    auto quicklist =
        std::make_unique<in_memory::QuickList>(list_max_listpack_bytes_);
    for (const auto& value : values) {
      if (!quicklist->RPush(value)) {
        return;
      }
    }
    quicklist_ = std::move(quicklist);
    return;
  }
  listpack_ = std::move(listpack);
  quicklist_.reset();
}
}  // namespace redis_simple::list
