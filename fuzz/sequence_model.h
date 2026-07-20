#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace redis_simple::fuzz {
inline size_t RemoveMatchesFromModel(std::vector<std::string>* model,
                                     std::string_view value, size_t limit,
                                     bool from_tail) {
  size_t removed = 0;
  if (from_tail && limit != 0) {
    for (size_t index = model->size(); index > 0 && removed < limit;) {
      --index;
      if ((*model)[index] == value) {
        model->erase(model->begin() + static_cast<std::ptrdiff_t>(index));
        ++removed;
      }
    }
    return removed;
  }

  for (size_t index = 0;
       index < model->size() && (limit == 0 || removed < limit);) {
    if ((*model)[index] == value) {
      model->erase(model->begin() + static_cast<std::ptrdiff_t>(index));
      ++removed;
    } else {
      ++index;
    }
  }
  return removed;
}
}  // namespace redis_simple::fuzz
