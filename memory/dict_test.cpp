#include "memory/dict.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>

namespace redis_simple {
namespace in_memory {
class DictStrTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    dict_str = Dict<std::string, std::string>::Init();
  }
  static std::unique_ptr<Dict<std::string, std::string>> dict_str;
};

std::unique_ptr<Dict<std::string, std::string>> DictStrTest::dict_str = nullptr;

TEST_F(DictStrTest, Init) {
  dict_str = Dict<std::string, std::string>::Init();
  ASSERT_TRUE(dict_str);
  ASSERT_EQ(dict_str->Size(), 0);
}

TEST_F(DictStrTest, Insert) {
  bool status = dict_str->Insert("key", "val");
  ASSERT_TRUE(status);
  ASSERT_EQ(dict_str->Size(), 1);

  // insert a new key
  const std::optional<std::string>& opt1 = dict_str->Get("key");
  ASSERT_TRUE(opt1.has_value());
  ASSERT_EQ(opt1.value_or(""), "val");
  ASSERT_EQ(dict_str->Size(), 1);

  // update the key
  dict_str->Set("key", "val_update");
  ASSERT_EQ(dict_str->Size(), 1);

  const std::optional<std::string>& opt2 = dict_str->Get("key");
  ASSERT_TRUE(opt2.has_value());
  ASSERT_EQ(opt2.value_or(""), "val_update");
  ASSERT_EQ(dict_str->Size(), 1);
}

TEST_F(DictStrTest, Delete) {
  bool status = dict_str->Delete("key");
  ASSERT_TRUE(status);
  ASSERT_EQ(dict_str->Size(), 0);

  const std::optional<std::string>& opt = dict_str->Get("key");
  ASSERT_FALSE(opt.has_value());
  ASSERT_EQ(opt, std::nullopt);
  ASSERT_EQ(dict_str->Size(), 0);
}

class DictIntTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { dict_int = Dict<int, int>::Init(); }
  static std::unique_ptr<Dict<int, int>> dict_int;
};

std::unique_ptr<Dict<int, int>> DictIntTest::dict_int = nullptr;

TEST_F(DictIntTest, Init) {
  const Dict<int, int>::DictType& type = {
      .hashFunction = [](const int i) { return i; },
  };
  ASSERT_TRUE(dict_int);
  ASSERT_EQ(dict_int->Size(), 0);
}

TEST_F(DictIntTest, BatchInsert) {
  for (int i = 0; i < 129; ++i) {
    dict_int->Insert(i, i);
  }
  ASSERT_EQ(dict_int->Size(), 129);
  const std::optional<int>& opt = dict_int->Get(96);
  ASSERT_TRUE(opt.has_value());
  ASSERT_EQ(opt.value_or(0), 96);
}

TEST_F(DictIntTest, Iterator) {
  in_memory::Dict<int, int>::Iterator it(dict_int.get());
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

TEST_F(DictIntTest, Clear) {
  dict_int->Clear();
  ASSERT_EQ(dict_int->Size(), 0);

  const std::optional<int>& opt = dict_int->Get(96);
  ASSERT_FALSE(opt.has_value());
  ASSERT_EQ(opt, std::nullopt);
}
}  // namespace in_memory
}  // namespace redis_simple
