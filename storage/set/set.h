#pragma once

#include <memory>
#include <string>
#include <vector>

#include "memory/dict.h"
#include "memory/intset.h"
#include "memory/listpack.h"

namespace redis_simple {
namespace set {
class Set {
 public:
  enum class Encoding {
    kIntSet,
    kListPack,
    kDict,
  };

  static Set* Init() { return new Set(); }
  bool Add(const std::string& value);
  bool HasMember(const std::string& value) const;
  std::vector<std::string> ListAllMembers() const;
  bool Remove(const std::string& value);
  size_t Size() const;
  Encoding GetEncoding() const;

 private:
  enum class SetEncodingType {
    IntSet = 1,
    ListPack = 2,
    Dict = 3,
  };
  static constexpr size_t IntSetMaxEntries = 512;
  static constexpr size_t ListPackMaxEntries = 128;
  static constexpr size_t ListPackElementMaxLength = 64;
  Set();
  bool IntSetAddAndMaybeConvert(const std::string& value);
  bool ListPackAddAndMaybeConvert(const std::string& value);
  bool DictAdd(const std::string& value);
  void MaybeConvertIntsetToDict();
  void ConvertIntSetToDict(size_t capacity);
  bool MaybeConvertIntSetToListPack(const std::string& val);
  void ConvertIntSetToListPack(const std::string& val);
  void ConvertListPackToDict(size_t capacity);
  std::vector<std::string> ListIntSetMembers() const;
  std::vector<std::string> ListListPackMembers() const;
  std::vector<std::string> ListDictMembers() const;
  SetEncodingType encoding_;
  std::unique_ptr<in_memory::IntSet> intset_;
  std::unique_ptr<in_memory::ListPack> listpack_;
  std::unique_ptr<in_memory::Dict<std::string, nullptr_t>> dict_;
};
}  // namespace set
}  // namespace redis_simple
