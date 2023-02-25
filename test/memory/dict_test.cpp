#include "src/memory/dict.h"

#include <gtest/gtest.h>

#include <string>

namespace redis_simple {
namespace in_memory {
std::unique_ptr<Dict<std::string, std::string>> dict_str;
std::unique_ptr<Dict<int, int>> dict_int;

TEST(DictStrTest, Init) {
  dict_str = Dict<std::string, std::string>::init();
  ASSERT_TRUE(dict_str);
  ASSERT_EQ(dict_str->size(), 0);
}

TEST(DictStrTest, Insert) {
  DictStatus status = dict_str->add("key", "val");
  ASSERT_EQ(status, DictStatus::dictOK);
  ASSERT_EQ(dict_str->size(), 1);

  const Dict<std::string, std::string>::DictEntry* entry =
      dict_str->find("key");
  ASSERT_TRUE(entry);
  ASSERT_EQ(entry->val, "val");
  ASSERT_EQ(dict_str->size(), 1);
}

TEST(DictStrTest, Update) {
  DictStatus status = dict_str->replace("key", "val_update");
  ASSERT_EQ(status, DictStatus::dictOK);
  ASSERT_EQ(dict_str->size(), 1);

  const Dict<std::string, std::string>::DictEntry* entry =
      dict_str->find("key");
  ASSERT_TRUE(entry);
  ASSERT_EQ(entry->val, "val_update");
  ASSERT_EQ(dict_str->size(), 1);
}

TEST(DictStrTest, Delete) {
  DictStatus status = dict_str->del("key");
  ASSERT_EQ(status, DictStatus::dictOK);
  ASSERT_EQ(dict_str->size(), 0);

  const Dict<std::string, std::string>::DictEntry* entry =
      dict_str->find("key");
  ASSERT_FALSE(entry);
  ASSERT_EQ(dict_str->size(), 0);
}

TEST(DictStrTest, Unlink) {
  DictStatus status = dict_str->add("key", "val");
  ASSERT_EQ(status, DictStatus::dictOK);
  ASSERT_EQ(dict_str->size(), 1);

  Dict<std::string, std::string>::DictEntry* de = dict_str->unlink("key");
  ASSERT_TRUE(de);
  ASSERT_EQ(de->key, "key");
  ASSERT_EQ(de->val, "val");
  ASSERT_EQ(dict_str->size(), 0);
}

TEST(DictIntTest, Init) {
  dict_int = Dict<int, int>::init();
  dict_int->getType()->hashFunction = [](const int i) { return i; };
  ASSERT_TRUE(dict_int);
  ASSERT_EQ(dict_int->size(), 0);
}

TEST(DictIntTest, BatchInsert) {
  for (int i = 0; i < 100; ++i) {
    dict_int->add(i, i);
  }
  ASSERT_EQ(dict_int->size(), 100);
  const Dict<int, int>::DictEntry* entry = dict_int->find(96);
  ASSERT_TRUE(entry);
  ASSERT_EQ(entry->val, 96);
}

TEST(DictIntTest, Clear) {
  dict_int->clear();
  ASSERT_EQ(dict_int->size(), 0);

  const Dict<int, int>::DictEntry* entry = dict_int->find(96);
  ASSERT_FALSE(entry);
}
}  // namespace in_memory
}  // namespace redis_simple
