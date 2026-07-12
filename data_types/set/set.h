#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "memory/dict.h"
#include "memory/intset.h"
#include "memory/listpack.h"

namespace redis_simple::set {
class Set {
 public:
  enum class Encoding {
    kIntSet,
    kListPack,
    kDict,
  };

  static std::unique_ptr<Set> Create() {
    return std::unique_ptr<Set>(new Set());
  }
  bool Add(const std::string& value);
  bool HasMember(const std::string& value) const;
  bool HasMember(std::string_view value) const;
  bool HasMember(const char* value) const {
    return HasMember(std::string_view(value));
  }
  std::vector<std::string> ListAllMembers() const;
  template <typename Visitor>
  bool ForEachMember(Visitor&& visitor) const;
  bool Remove(const std::string& value);
  size_t Size() const;
  Encoding Encoding() const;

 private:
  static constexpr size_t kIntSetMaxEntries = 512;
  static constexpr size_t kListPackMaxEntries = 128;
  static constexpr size_t kListPackElementMaxLength = 64;
  Set();
  bool IntSetAddAndMaybeConvert(const std::string& value);
  bool ListPackAddAndMaybeConvert(const std::string& value);
  bool DictAdd(const std::string& value);
  void MaybeConvertIntsetToDict();
  void ConvertIntSetToDict(size_t capacity);
  bool MaybeConvertIntSetToListPack(const std::string& val);
  void ConvertIntSetToListPack(const std::string& val);
  void ConvertListPackToDict(size_t capacity);
  enum Encoding encoding_;
  std::unique_ptr<in_memory::IntSet> intset_;
  std::unique_ptr<in_memory::ListPack> listpack_;
  std::unique_ptr<in_memory::Dict<std::string, nullptr_t>> dict_;
};

template <typename Visitor>
bool Set::ForEachMember(Visitor&& visitor) const {
  const size_t size = Size();
  if (size == 0) {
    return true;
  }
  if (encoding_ == Encoding::kIntSet) {
    auto it = in_memory::IntSet::Iterator(intset_.get());
    it.SeekToFirst();
    while (it.Valid()) {
      const std::string member = std::to_string(it.Value());
      if (!visitor(member)) {
        return false;
      }
      it.Next();
    }
    return true;
  }
  if (encoding_ == Encoding::kListPack) {
    return listpack_->ForEach(0, size - 1, visitor);
  }
  if (encoding_ == Encoding::kDict) {
    auto it = in_memory::Dict<std::string, nullptr_t>::Iterator(dict_.get());
    it.SeekToFirst();
    while (it.Valid()) {
      if (!visitor(it.Key())) {
        return false;
      }
      it.Next();
    }
    return true;
  }
  throw std::invalid_argument("unknown encoding type");
}
}  // namespace redis_simple::set
