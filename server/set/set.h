#pragma once

#include <memory>
#include <string>

#include "memory/dict.h"
#include "memory/intset.h"
#include "memory/listpack.h"

namespace redis_simple {
namespace set {
class Set {
  enum class SetEncodingType {
    setEncodingIntSet = 1,
    setEncodingListPack = 2,
    setEncodingDict = 3,
  };

 public:
  static Set* Init() { return new Set(); }
  bool Add(const std::string& value);
  bool HasMember(const std::string& value) const;
  std::vector<std::string> ListAllMembers() const;
  bool Remove(const std::string& value);
  size_t Size() const;

 private:
  static constexpr size_t IntSetMaxEntries = 512;
  static constexpr size_t ListPackMaxEntries = 128;
  explicit Set();
  void MaybeConvertIntsetToDict();
  void ConvertIntSetToDict(size_t capacity);
  bool MaybeConvertIntSetToListPack(const std::string& val);
  void ConvertIntSetToListPack(const std::string& val);
  void ConvertListPackToDict(const std::string& val);
  std::vector<std::string> ListIntSetMembers() const;
  std::vector<std::string> ListListPackMembers() const;
  std::vector<std::string> ListDictMembers() const;
  /* set encoding, could either be intset or dict */
  SetEncodingType encoding_;
  std::unique_ptr<in_memory::IntSet> intset_;
  std::unique_ptr<in_memory::ListPack> listpack_;
  std::unique_ptr<in_memory::Dict<std::string, nullptr_t>> dict_;
};
}  // namespace set
}  // namespace redis_simple
