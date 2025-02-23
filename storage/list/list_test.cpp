#include "storage/list/list.h"

#include <gtest/gtest.h>

namespace redis_simple {
namespace list {
class ListTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { list = List::Init(); }
  static void TearDownTestSuit() {
    delete list;
    list = nullptr;
  }
  static List* list;
};

List* ListTest::list = nullptr;

TEST_F(ListTest, Push) {
  ASSERT_TRUE(list->LPush("12345"));
  ASSERT_TRUE(list->RPush("test string 0"));
  ASSERT_TRUE(list->LPush("hello world"));
  ASSERT_TRUE(list->RPush("test string 1"));
  ASSERT_TRUE(list->LPush("1234567"));
  ASSERT_EQ(list->Size(), 5);
}

TEST_F(ListTest, Pop) {
  ASSERT_EQ(list->LPop(), "1234567");
  ASSERT_EQ(list->RPop(), "test string 1");
  ASSERT_EQ(list->Size(), 3);
}
}  // namespace list
}  // namespace redis_simple
