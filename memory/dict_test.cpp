#include "memory/dict.h"

#include <gtest/gtest.h>

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
  bool status = dict_str->Add("key", "val");
  ASSERT_TRUE(status);
  ASSERT_EQ(dict_str->Size(), 1);

  const Dict<std::string, std::string>::DictEntry* entry =
      dict_str->Find("key");
  ASSERT_TRUE(entry);
  ASSERT_EQ(entry->val, "val");
  ASSERT_EQ(dict_str->Size(), 1);
}

TEST_F(DictStrTest, Update) {
  dict_str->Replace("key", "val_update");
  ASSERT_EQ(dict_str->Size(), 1);

  const Dict<std::string, std::string>::DictEntry* entry =
      dict_str->Find("key");
  ASSERT_TRUE(entry);
  ASSERT_EQ(entry->val, "val_update");
  ASSERT_EQ(dict_str->Size(), 1);
}

TEST_F(DictStrTest, Delete) {
  bool status = dict_str->Delete("key");
  ASSERT_TRUE(status);
  ASSERT_EQ(dict_str->Size(), 0);

  const Dict<std::string, std::string>::DictEntry* entry =
      dict_str->Find("key");
  ASSERT_FALSE(entry);
  ASSERT_EQ(dict_str->Size(), 0);
}

TEST_F(DictStrTest, Unlink) {
  bool status = dict_str->Add("key", "val");
  ASSERT_TRUE(status);
  ASSERT_EQ(dict_str->Size(), 1);

  Dict<std::string, std::string>::DictEntry* de = dict_str->Unlink("key");
  ASSERT_TRUE(de);
  ASSERT_EQ(de->key, "key");
  ASSERT_EQ(de->val, "val");
  ASSERT_EQ(dict_str->Size(), 0);
}

TEST_F(DictStrTest, AddOrFind) {
  bool status = dict_str->Add("key", "val");
  ASSERT_TRUE(status);
  ASSERT_EQ(dict_str->Size(), 1);

  /* call the method with an existing key */
  const Dict<std::string, std::string>::DictEntry* entry1 =
      dict_str->AddOrFind("key");
  ASSERT_EQ(entry1->key, "key");
  ASSERT_EQ(entry1->val, "val");

  /* call the method with a new key */
  const Dict<std::string, std::string>::DictEntry* entry2 =
      dict_str->AddOrFind("key1");
  ASSERT_EQ(dict_str->Size(), 2);
  ASSERT_EQ(entry2->key, "key1");
  ASSERT_EQ(entry2->val.size(), 0);
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
  for (int i = 0; i < 100; ++i) {
    dict_int->Add(i, i);
  }
  ASSERT_EQ(dict_int->Size(), 100);
  const Dict<int, int>::DictEntry* entry = dict_int->Find(96);
  ASSERT_TRUE(entry);
  ASSERT_EQ(entry->val, 96);
}

TEST_F(DictIntTest, AddOrFind) {
  /* call the method with an existing key */
  const Dict<int, int>::DictEntry* entry1 = dict_int->AddOrFind(99);
  ASSERT_EQ(dict_int->Size(), 100);
  ASSERT_EQ(entry1->key, 99);
  ASSERT_EQ(entry1->val, 99);

  /* call the method with a new key */
  const Dict<int, int>::DictEntry* entry2 = dict_int->AddOrFind(100);
  ASSERT_EQ(dict_int->Size(), 101);
  ASSERT_EQ(entry2->key, 100);
  ASSERT_EQ(entry2->val, 0);
}

TEST_F(DictIntTest, Clear) {
  dict_int->Clear();
  ASSERT_EQ(dict_int->Size(), 0);

  const Dict<int, int>::DictEntry* entry = dict_int->Find(96);
  ASSERT_FALSE(entry);
}
}  // namespace in_memory
}  // namespace redis_simple
