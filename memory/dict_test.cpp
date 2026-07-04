#include "memory/dict.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>

namespace redis_simple::in_memory {
TEST(DictStrTest, Init) {
  auto dict_str = Dict<std::string, std::string>::Create();
  ASSERT_TRUE(dict_str);
  ASSERT_EQ(dict_str->Size(), 0);
}

TEST(DictStrTest, Insert) {
  auto dict_str = Dict<std::string, std::string>::Create();
  auto status = dict_str->Insert("key", "val");
  ASSERT_TRUE(status);
  ASSERT_EQ(dict_str->Size(), 1);

  // Insert a new key.
  const auto opt1 = dict_str->Get("key");
  ASSERT_TRUE(opt1.has_value());
  ASSERT_EQ(opt1.value_or(""), "val");
  ASSERT_EQ(dict_str->Size(), 1);

  // Update the key.
  dict_str->Set("key", "val_update");
  ASSERT_EQ(dict_str->Size(), 1);

  const auto opt2 = dict_str->Get("key");
  ASSERT_TRUE(opt2.has_value());
  ASSERT_EQ(opt2.value_or(""), "val_update");
  ASSERT_EQ(dict_str->Size(), 1);
}

TEST(DictStrTest, InsertDuplicate) {
  auto dict_str = Dict<std::string, std::string>::Create();
  ASSERT_TRUE(dict_str->Insert("key", "val"));
  ASSERT_FALSE(dict_str->Insert("key", "new_val"));
  ASSERT_EQ(dict_str->Size(), 1);

  const auto result = dict_str->Get("key");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result.value_or(""), "val");
}

TEST(DictStrTest, Delete) {
  auto dict_str = Dict<std::string, std::string>::Create();
  ASSERT_TRUE(dict_str->Insert("key", "val"));

  auto status = dict_str->Delete("key");
  ASSERT_TRUE(status);
  ASSERT_EQ(dict_str->Size(), 0);

  const auto result = dict_str->Get("key");
  ASSERT_FALSE(result.has_value());
  ASSERT_EQ(result, std::nullopt);
  ASSERT_EQ(dict_str->Size(), 0);
}

namespace {
std::unique_ptr<Dict<int, int>> MakeIntDictWithEntries() {
  auto dict_int = Dict<int, int>::Create();
  for (int i = 0; i < 129; ++i) {
    dict_int->Insert(i, i);
  }
  return dict_int;
}
}  // namespace

TEST(DictIntTest, Init) {
  auto dict_int = Dict<int, int>::Create();
  ASSERT_TRUE(dict_int);
  ASSERT_EQ(dict_int->Size(), 0);
}

TEST(DictIntTest, BatchInsert) {
  auto dict_int = MakeIntDictWithEntries();
  ASSERT_EQ(dict_int->Size(), 129);
  const auto result = dict_int->Get(96);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result.value_or(0), 96);
}

TEST(DictIntTest, Iterator) {
  auto dict_int = MakeIntDictWithEntries();
  auto it = in_memory::Dict<int, int>::Iterator(dict_int.get());
  it.SeekToFirst();
  ASSERT_TRUE(it.Valid());
  ASSERT_EQ(it.Key(), 128);
  ASSERT_EQ(it.Value(), 128);

  it.Next();
  ASSERT_TRUE(it.Valid());
  ASSERT_EQ(it.Key(), 0);
  ASSERT_EQ(it.Value(), 0);

  it.Next();
  ASSERT_TRUE(it.Valid());
  ASSERT_EQ(it.Key(), 1);
  ASSERT_EQ(it.Value(), 1);

  it.Next();
  ASSERT_TRUE(it.Valid());
  ASSERT_EQ(it.Key(), 2);
  ASSERT_EQ(it.Value(), 2);

  it.SeekToLast();
  ASSERT_TRUE(it.Valid());
  ASSERT_EQ(it.Key(), 127);
  ASSERT_EQ(it.Value(), 127);

  it.Next();
  ASSERT_FALSE(it.Valid());
}

TEST(DictIntTest, Clear) {
  auto dict_int = MakeIntDictWithEntries();
  dict_int->Clear();
  ASSERT_EQ(dict_int->Size(), 0);

  const auto result = dict_int->Get(96);
  ASSERT_FALSE(result.has_value());
  ASSERT_EQ(result, std::nullopt);

  ASSERT_TRUE(dict_int->Insert(1, 10));
  ASSERT_EQ(dict_int->Size(), 1);
  ASSERT_EQ(dict_int->Get(1), 10);
}
}  // namespace redis_simple::in_memory
