#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "memory/dict.h"
#include "memory/listpack.h"

namespace redis_simple::hash {
class Hash {
 public:
  enum class Encoding {
    kListPack,
    kDict,
  };

  struct Entry {
    std::string field;
    std::string value;
  };

  static std::unique_ptr<Hash> Create() {
    return std::unique_ptr<Hash>(new Hash());
  }

  bool Set(const std::string& field, const std::string& value);
  std::optional<std::string> Get(const std::string& field) const;
  bool Delete(const std::string& field);
  bool Exists(const std::string& field) const;
  size_t Size() const;
  std::vector<Entry> Entries() const;
  template <typename Visitor>
  bool ForEachEntry(Visitor&& visitor) const;
  template <typename Visitor>
  bool ForEachField(Visitor&& visitor) const;
  template <typename Visitor>
  bool ForEachValue(Visitor&& visitor) const;
  Encoding Encoding() const { return encoding_; }

 private:
  Hash();
  bool CanAppendToListPack(const std::string& field,
                           const std::string& value) const;
  bool SetListPack(const std::string& field, const std::string& value);
  bool SetDict(const std::string& field, const std::string& value);
  void ConvertListPackToDict(size_t capacity);
  std::optional<size_t> FindListPackField(const std::string& field) const;

  enum Encoding encoding_;
  std::unique_ptr<in_memory::ListPack> listpack_;
  std::unique_ptr<in_memory::Dict<std::string, std::string>> dict_;
};

template <typename Visitor>
bool Hash::ForEachEntry(Visitor&& visitor) const {
  if (Size() == 0) {
    return true;
  }
  if (encoding_ == Encoding::kListPack) {
    return listpack_->ForEachPair(visitor);
  }
  if (encoding_ == Encoding::kDict) {
    auto it = in_memory::Dict<std::string, std::string>::Iterator(dict_.get());
    it.SeekToFirst();
    while (it.Valid()) {
      if (!visitor(it.Key(), it.Value())) {
        return false;
      }
      it.Next();
    }
    return true;
  }
  throw std::invalid_argument("unknown hash encoding type");
}

template <typename Visitor>
bool Hash::ForEachField(Visitor&& visitor) const {
  return ForEachEntry(
      [&visitor](std::string_view field, std::string_view /*value*/) {
        return visitor(field);
      });
}

template <typename Visitor>
bool Hash::ForEachValue(Visitor&& visitor) const {
  return ForEachEntry(
      [&visitor](std::string_view /*field*/, std::string_view value) {
        return visitor(value);
      });
}
}  // namespace redis_simple::hash
