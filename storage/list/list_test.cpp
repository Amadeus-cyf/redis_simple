#include "storage/list/list.h"

#include <gtest/gtest.h>

#include <memory>
#include <optional>

namespace redis_simple {
namespace list {
class ListTest : public testing::Test {
 protected:
  void SetUp() override { list = std::unique_ptr<List>(List::Init()); }

  std::unique_ptr<List> list;
};

TEST_F(ListTest, Push) {
  ASSERT_TRUE(list->LPush("12345"));
  ASSERT_TRUE(list->RPush("test string 0"));
  ASSERT_TRUE(list->LPush("hello world"));
  ASSERT_TRUE(list->RPush("test string 1"));
  ASSERT_TRUE(list->LPush("1234567"));
  ASSERT_EQ(list->Size(), 5);
  ASSERT_EQ(list->GetEncoding(), List::Encoding::kListPack);
  ASSERT_EQ(list->NodeCount(), 0);
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
  ASSERT_EQ(list->GetEncoding(), List::Encoding::kListPack);
}

TEST_F(ListTest, PopEmpty) {
  ASSERT_EQ(list->LPop(), std::nullopt);
  ASSERT_EQ(list->RPop(), std::nullopt);
  ASSERT_EQ(list->Size(), 0);
  ASSERT_EQ(list->NodeCount(), 0);
  ASSERT_EQ(list->GetEncoding(), List::Encoding::kListPack);
}

TEST(ListEncodingTest, ConvertsFromListPackToQuickListWhenListGrows) {
  auto list = std::unique_ptr<List>(List::Init(96));
  const std::string value(32, 'x');

  ASSERT_TRUE(list->RPush(value + "1"));
  ASSERT_TRUE(list->RPush(value + "2"));
  ASSERT_EQ(list->GetEncoding(), List::Encoding::kListPack);

  ASSERT_TRUE(list->RPush(value + "3"));

  ASSERT_EQ(list->GetEncoding(), List::Encoding::kQuickList);
  ASSERT_EQ(list->Size(), 3);
  ASSERT_EQ(list->NodeCount(), 2);
  ASSERT_EQ(list->LPop(), value + "1");
  ASSERT_EQ(list->LPop(), value + "2");
  ASSERT_EQ(list->LPop(), value + "3");
}

TEST(ListEncodingTest, ConvertsBackToListPackAfterShrinking) {
  auto list = std::unique_ptr<List>(List::Init(96));
  const std::string value(32, 'y');

  ASSERT_TRUE(list->RPush(value + "1"));
  ASSERT_TRUE(list->RPush(value + "2"));
  ASSERT_TRUE(list->RPush(value + "3"));
  ASSERT_EQ(list->GetEncoding(), List::Encoding::kQuickList);

  ASSERT_EQ(list->RPop(), value + "3");
  ASSERT_EQ(list->GetEncoding(), List::Encoding::kQuickList);
  ASSERT_EQ(list->RPop(), value + "2");

  ASSERT_EQ(list->GetEncoding(), List::Encoding::kListPack);
  ASSERT_EQ(list->Size(), 1);
  ASSERT_EQ(list->NodeCount(), 0);
  ASSERT_EQ(list->LPop(), value + "1");
}
}  // namespace list
}  // namespace redis_simple
