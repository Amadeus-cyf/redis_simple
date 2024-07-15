#pragma once

#include <memory>
#include <string>

#include "memory/dict.h"
#include "memory/intset.h"

namespace redis_simple {
namespace set {
class Set {
  enum class SetEncodingType {
    setEncodingIntSet = 1,
    setEncodingDict = 2,
  };

 public:
  static Set* Init() { return new Set(); }
  bool Add(const std::string& value);
  bool HasMember(const std::string& value) const;
  std::vector<std::string> ListAllMembers() const;
  bool Remove(const std::string& value);
  size_t Size() const;

 private:
  explicit Set();
  bool IsInt64(const std::string& value) const;
  void ConvertIntSetToDict();
  std::vector<std::string> ListIntSetMembers() const;
  std::vector<std::string> ListDictMembers() const;
  /* set encoding, could either be intset or dict */
  SetEncodingType encoding_;
  std::unique_ptr<in_memory::IntSet> intset_;
  std::unique_ptr<in_memory::Dict<std::string, nullptr_t>> dict_;
};
}  // namespace set
}  // namespace redis_simple
