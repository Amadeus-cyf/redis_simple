#include "data_types/list/list.h"

#include <algorithm>
#include <cstdint>

namespace redis_simple::list {
namespace {
constexpr size_t kEntryOverheadEstimate = 8;

bool HasRemoveLimit(size_t limit) { return limit != 0; }

size_t CountMatches(const List& list, size_t size, std::string_view value) {
  size_t matches = 0;
  list.ForEach(0, size - 1, [&matches, value](std::string_view current) {
    if (current == value) {
      ++matches;
    }
    return true;
  });
  return matches;
}

size_t LeadingMatchesToKeep(const List& list, size_t size,
                            std::string_view value, size_t limit,
                            List::RemoveDirection direction) {
  if (direction == List::RemoveDirection::kFromHead || !HasRemoveLimit(limit)) {
    return 0;
  }
  // Tail removals rebuild from left to right, so keep the leading matching
  // elements and remove only the requested final matches.
  const size_t matches = CountMatches(list, size, value);
  return matches > limit ? matches - limit : 0;
}

struct RemoveState {
  std::string_view target;
  size_t limit;
  List::RemoveDirection direction;
  size_t leading_matches_to_keep;
  size_t kept_leading_matches{0};
  size_t removed{0};

  bool ShouldRemove(std::string_view current) {
    if (current != target) {
      return false;
    }
    if (direction == List::RemoveDirection::kFromTail &&
        HasRemoveLimit(limit) &&
        kept_leading_matches < leading_matches_to_keep) {
      ++kept_leading_matches;
      return false;
    }
    if (!HasRemoveLimit(limit) || removed < limit) {
      ++removed;
      return true;
    }
    return false;
  }
};
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

  auto replacement = List::Create(list_max_listpack_bytes_);
  size_t current_index = 0;
  const bool replaced = ForEach(
      0, size - 1,
      [&replacement, &current_index, index, value](std::string_view current) {
        const auto next = current_index == index ? value : current;
        ++current_index;
        return replacement->RPush(next);
      });
  return replaced ? AdoptReplacement(std::move(replacement)) : false;
}

std::optional<size_t> List::Remove(std::string_view value, size_t limit,
                                   RemoveDirection direction) {
  const size_t size = Size();
  if (size == 0) {
    return 0;
  }

  RemoveState state{value, limit, direction,
                    LeadingMatchesToKeep(*this, size, value, limit, direction)};
  auto replacement = List::Create(list_max_listpack_bytes_);
  const bool rebuilt =
      ForEach(0, size - 1, [&replacement, &state](std::string_view current) {
        if (state.ShouldRemove(current)) {
          return true;
        }
        return replacement->RPush(current);
      });
  if (!rebuilt) {
    return std::nullopt;
  }
  if (state.removed == 0) {
    return 0;
  }
  if (!AdoptReplacement(std::move(replacement))) {
    return std::nullopt;
  }
  return state.removed;
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

  auto replacement = List::Create(list_max_listpack_bytes_);
  const bool rebuilt =
      ForEach(start, stop, [&replacement](std::string_view value) {
        return replacement->RPush(value);
      });
  return rebuilt ? AdoptReplacement(std::move(replacement)) : false;
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
