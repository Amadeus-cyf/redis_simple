#include "storage/list/list.h"

#include <gtest/gtest.h>

#include <optional>

namespace redis_simple {
namespace list {
class ListTest : public testing::Test {
 protected:
  void SetUp() override { list = List::Init(); }
  void TearDown() override {
    delete list;
    list = nullptr;
  }
  List* list = nullptr;
};

TEST_F(ListTest, Push) {
  ASSERT_TRUE(list->LPush("12345"));
  ASSERT_TRUE(list->RPush("test string 0"));
  ASSERT_TRUE(list->LPush("hello world"));
  ASSERT_TRUE(list->RPush("test string 1"));
  ASSERT_TRUE(list->LPush("1234567"));
  ASSERT_EQ(list->Size(), 5);
  ASSERT_EQ(list->NodeCount(), 1);
}

TEST_F(ListTest, Pop) {
  ASSERT_TRUE(list->LPush("12345"));
  ASSERT_TRUE(list->RPush("test string 0"));
  ASSERT_TRUE(list->LPush("hello world"));
  ASSERT_TRUE(list->RPush("test string 1"));
  ASSERT_TRUE(list->LPush("1234567"));

  ASSERT_EQ(list->LPop(), "1234567");
  ASSERT_EQ(list->RPop(), "test string 1");
  ASSERT_EQ(list->Size(), 3);
}

TEST_F(ListTest, PopEmpty) {
  ASSERT_EQ(list->LPop(), std::nullopt);
  ASSERT_EQ(list->RPop(), std::nullopt);
  ASSERT_EQ(list->Size(), 0);
  ASSERT_EQ(list->NodeCount(), 0);
}
}  // namespace list
}  // namespace redis_simple
