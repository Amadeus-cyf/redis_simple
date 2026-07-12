#include "data_types/list/list.h"

#include <algorithm>
#include <cstdint>

namespace redis_simple::list {
namespace {
constexpr size_t kEntryOverheadEstimate = 8;

bool HasRemoveLimit(size_t limit) { return limit != 0; }

bool ListPackEntryEquals(const in_memory::ListPack* const listpack,
                         size_t index, std::string_view value) {
  size_t len = 0;
  const auto* data = listpack->Get(index, &len);
  return data != nullptr &&
         std::string_view(reinterpret_cast<const char*>(data), len) == value;
}

ssize_t NextAfterDelete(const in_memory::ListPack* const listpack,
                        size_t deleted_index) {
  return deleted_index < listpack->TotalBytes() - 1
             ? static_cast<ssize_t>(deleted_index)
             : -1;
}

in_memory::QuickList::RemoveDirection ToQuickListRemoveDirection(
    List::RemoveDirection direction) {
  return direction == List::RemoveDirection::kFromTail
             ? in_memory::QuickList::RemoveDirection::kFromTail
             : in_memory::QuickList::RemoveDirection::kFromHead;
}
}  // namespace

List::List(size_t list_max_listpack_bytes)
    : listpack_(std::make_unique<in_memory::ListPack>()),
      quicklist_(nullptr),
      list_max_listpack_bytes_(list_max_listpack_bytes) {}

bool List::LPush(std::string_view value) { return Push(value, true); }

bool List::RPush(std::string_view value) { return Push(value, false); }

std::optional<std::string> List::RPop() { return Pop(false); }

std::optional<std::string> List::LPop() { return Pop(true); }

std::optional<std::string> List::At(size_t index) const {
  std::optional<std::string> result;
  ForEach(index, index, [&result](std::string_view value) {
    result.emplace(value);
    return true;
  });
  return result;
}

bool List::Set(size_t index, std::string_view value) {
  const size_t size = Size();
  if (index >= size) {
    return false;
  }

  if (listpack_) {
    const auto listpack_index = listpack_->IndexAt(index);
    if (!listpack_index.has_value() ||
        !listpack_->Replace(*listpack_index, value)) {
      return false;
    }
    if (listpack_->TotalBytes() > list_max_listpack_bytes_) {
      return ConvertListPackToQuickList();
    }
    return true;
  }

  return quicklist_->Set(index, value);
}

std::optional<size_t> List::Remove(std::string_view value, size_t limit,
                                   RemoveDirection direction) {
  const size_t size = Size();
  if (size == 0) {
    return 0;
  }

  if (listpack_) {
    return RemoveFromListPack(value, limit, direction);
  }

  return quicklist_->Remove(value, limit,
                            ToQuickListRemoveDirection(direction));
}

bool List::Trim(size_t start, size_t stop) {
  const size_t size = Size();
  if (size == 0 || start > stop || start >= size) {
    return AdoptReplacement(List::Create(list_max_listpack_bytes_));
  }
  stop = std::min(stop, size - 1);
  if (start == 0 && stop == size - 1) {
    return true;
  }

  if (listpack_) {
    TrimListPack(start, stop);
    return true;
  }

  return quicklist_->Trim(start, stop);
}

std::optional<size_t> List::RemoveFromListPack(std::string_view value,
                                               size_t limit,
                                               RemoveDirection direction) {
  size_t removed = 0;
  if (direction == RemoveDirection::kFromTail && HasRemoveLimit(limit)) {
    ssize_t idx = listpack_->Last();
    while (idx != -1) {
      const auto current_index = static_cast<size_t>(idx);
      idx = listpack_->Prev(current_index);
      if (ListPackEntryEquals(listpack_.get(), current_index, value)) {
        listpack_->Delete(current_index);
        ++removed;
        if (removed >= limit) {
          break;
        }
      }
    }
    return removed;
  }

  ssize_t idx = listpack_->First();
  while (idx != -1) {
    const auto current_index = static_cast<size_t>(idx);
    if (ListPackEntryEquals(listpack_.get(), current_index, value)) {
      listpack_->Delete(current_index);
      ++removed;
      if (HasRemoveLimit(limit) && removed >= limit) {
        break;
      }
      idx = NextAfterDelete(listpack_.get(), current_index);
      continue;
    }
    idx = listpack_->Next(current_index);
  }
  return removed;
}

void List::TrimListPack(size_t start, size_t stop) {
  const size_t size = listpack_->Size();
  const size_t tail_count = size - stop - 1;
  for (size_t i = 0; i < tail_count; ++i) {
    const auto idx = listpack_->IndexAt(stop + 1);
    if (!idx.has_value()) {
      break;
    }
    listpack_->Delete(*idx);
  }
  for (size_t i = 0; i < start; ++i) {
    const ssize_t idx = listpack_->First();
    if (idx == -1) {
      break;
    }
    listpack_->Delete(static_cast<size_t>(idx));
  }
}

size_t List::Size() const {
  return listpack_ ? listpack_->Size() : quicklist_->Size();
}

size_t List::NodeCount() const {
  return quicklist_ ? quicklist_->NodeCount() : 0;
}

std::vector<std::string> List::Range(size_t start, size_t stop) const {
  std::vector<std::string> values;
  const size_t size = Size();
  if (start > stop || start >= size) {
    return values;
  }
  stop = std::min(stop, size - 1);
  values.reserve(stop - start + 1);
  ForEach(start, stop, [&values](std::string_view value) {
    values.emplace_back(value);
    return true;
  });
  return values;
}

enum List::Encoding List::Encoding() const {
  return listpack_ ? Encoding::kListPack : Encoding::kQuickList;
}

bool List::Push(std::string_view value, bool head) {
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

bool List::AdoptReplacement(std::unique_ptr<List> replacement) {
  listpack_ = std::move(replacement->listpack_);
  quicklist_ = std::move(replacement->quicklist_);
  return true;
}

bool List::WouldExceedListpackLimit(std::string_view value) const {
  if (!listpack_) {
    return false;
  }
  return listpack_->TotalBytes() + value.size() + kEntryOverheadEstimate >
         list_max_listpack_bytes_;
}

bool List::ConvertListPackToQuickList() {
  auto quicklist =
      std::make_unique<in_memory::QuickList>(list_max_listpack_bytes_);
  const size_t size = listpack_->Size();
  const bool converted =
      size == 0 ||
      listpack_->ForEach(0, size - 1, [&quicklist](std::string_view value) {
        return quicklist->RPush(value);
      });
  if (!converted) {
    return false;
  }
  quicklist_ = std::move(quicklist);
  listpack_.reset();
  return true;
}

void List::TryConvertQuickListToListPack() {
  if (!quicklist_ || quicklist_->NodeCount() > 1) {
    return;
  }

  auto listpack = std::make_unique<in_memory::ListPack>();
  const size_t size = quicklist_->Size();
  const bool converted =
      size == 0 ||
      quicklist_->ForEach(0, size - 1, [&listpack](std::string_view value) {
        return listpack->Append(value);
      });
  if (!converted) {
    return;
  }
  if (listpack->TotalBytes() > list_max_listpack_bytes_ / 2) {
    return;
  }
  listpack_ = std::move(listpack);
  quicklist_.reset();
}
}  // namespace redis_simple::list
