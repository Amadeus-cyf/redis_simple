#include "data_types/list/list.h"

#include <algorithm>
#include <cstdint>
#include <limits>
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

std::optional<std::string> List::At(size_t index) const {
  const auto values = Range(index, index);
  return values.empty() ? std::nullopt
                        : std::optional<std::string>(values.front());
}

bool List::Set(size_t index, const std::string& value) {
  if (index >= Size()) {
    return false;
  }
  auto values = Range(0, Size() - 1);
  values[index] = value;
  return ReplaceAll(values);
}

size_t List::Remove(const std::string& value, int64_t count) {
  const size_t size = Size();
  if (size == 0) {
    return 0;
  }

  size_t limit = std::numeric_limits<size_t>::max();
  if (count > 0) {
    limit = static_cast<size_t>(count);
  } else if (count < 0 && count != std::numeric_limits<int64_t>::min()) {
    limit = static_cast<size_t>(-count);
  }
  size_t removed = 0;
  std::vector<std::string> kept;
  kept.reserve(size);
  const auto values = Range(0, size - 1);
  if (count >= 0) {
    for (const auto& current : values) {
      if (current == value && removed < limit) {
        ++removed;
        continue;
      }
      kept.push_back(current);
    }
  } else {
    std::vector<std::string> reversed_kept;
    reversed_kept.reserve(size);
    for (auto it = values.rbegin(); it != values.rend(); ++it) {
      if (*it == value && removed < limit) {
        ++removed;
        continue;
      }
      reversed_kept.push_back(*it);
    }
    kept.assign(reversed_kept.rbegin(), reversed_kept.rend());
  }

  if (removed > 0) {
    ReplaceAll(kept);
  }
  return removed;
}

bool List::Trim(size_t start, size_t stop) {
  const size_t size = Size();
  if (size == 0 || start > stop || start >= size) {
    return ReplaceAll({});
  }
  stop = std::min(stop, size - 1);
  return ReplaceAll(Range(start, stop));
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
  if (listpack_) {
    size_t index = 0;
    ssize_t listpack_index = listpack_->First();
    while (listpack_index != -1 && index <= stop) {
      if (index >= start) {
        auto value = listpack_->Get(static_cast<size_t>(listpack_index));
        if (value.has_value()) {
          values.push_back(*value);
        }
      }
      ++index;
      listpack_index = listpack_->Next(static_cast<size_t>(listpack_index));
    }
    return values;
  }
  return quicklist_->Range(start, stop);
}

enum List::Encoding List::Encoding() const {
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

bool List::ReplaceAll(const std::vector<std::string>& values) {
  listpack_ = std::make_unique<in_memory::ListPack>();
  quicklist_.reset();
  return std::all_of(values.begin(), values.end(),
                     [this](const auto& value) { return RPush(value); });
}

bool List::WouldExceedListpackLimit(const std::string& value) const {
  if (!listpack_) {
    return false;
  }
  return listpack_->TotalBytes() + value.size() + kEntryOverheadEstimate >
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
  if (listpack->TotalBytes() > list_max_listpack_bytes_ / 2) {
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
