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
  bool HasMember(const std::string& value);
  bool Remove(const std::string& value);
  size_t Size();

 private:
  explicit Set();
  bool IsInt64(const std::string& value);
  void ConvertIntSetToDict();
  /* set encoding, could either be intset or dict */
  SetEncodingType encoding_;
  std::unique_ptr<in_memory::IntSet> intset_;
  std::unique_ptr<in_memory::Dict<std::string, nullptr_t>> dict_;
};
}  // namespace set
}  // namespace redis_simple
