#pragma once

#include <sys/types.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
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
  Encoding Encoding() const { return encoding_; }

 private:
  static constexpr size_t kListPackMaxEntries = 128;
  static constexpr size_t kListPackMaxElementLength = 64;

  Hash();
  static bool CanStoreInListPack(const std::string& field,
                                 const std::string& value);
  bool CanAppendToListPack(const std::string& field,
                           const std::string& value) const;
  bool SetListPack(const std::string& field, const std::string& value);
  bool SetDict(const std::string& field, const std::string& value);
  void ConvertListPackToDict(size_t capacity);
  std::optional<ssize_t> FindListPackField(const std::string& field) const;
  std::vector<Entry> ListPackEntries() const;
  std::vector<Entry> DictEntries() const;

  enum Encoding encoding_;
  std::unique_ptr<in_memory::ListPack> listpack_;
  std::unique_ptr<in_memory::Dict<std::string, std::string>> dict_;
};
}  // namespace redis_simple::hash
