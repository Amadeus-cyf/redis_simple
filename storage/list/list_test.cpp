#include "storage/list/list.h"

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <vector>

namespace redis_simple::list {
class ListTest : public testing::Test {
 protected:
  void SetUp() override { list_ = List::Create(); }
  List* TestList() { return list_.get(); }

 private:
  std::unique_ptr<List> list_;
};

TEST_F(ListTest, Push) {
  ASSERT_TRUE(TestList()->LPush("12345"));
  ASSERT_TRUE(TestList()->RPush("test string 0"));
  ASSERT_TRUE(TestList()->LPush("hello world"));
  ASSERT_TRUE(TestList()->RPush("test string 1"));
  ASSERT_TRUE(TestList()->LPush("1234567"));
  ASSERT_EQ(TestList()->Size(), 5);
  ASSERT_EQ(TestList()->Encoding(), List::Encoding::kListPack);
  ASSERT_EQ(TestList()->NodeCount(), 0);
}

TEST_F(ListTest, Pop) {
  ASSERT_TRUE(TestList()->LPush("12345"));
  ASSERT_TRUE(TestList()->RPush("test string 0"));
  ASSERT_TRUE(TestList()->LPush("hello world"));
  ASSERT_TRUE(TestList()->RPush("test string 1"));
  ASSERT_TRUE(TestList()->LPush("1234567"));

  ASSERT_EQ(TestList()->LPop(), "1234567");
  ASSERT_EQ(TestList()->RPop(), "test string 1");
  ASSERT_EQ(TestList()->Size(), 3);
  ASSERT_EQ(TestList()->Encoding(), List::Encoding::kListPack);
}

TEST_F(ListTest, PopEmpty) {
  ASSERT_EQ(TestList()->LPop(), std::nullopt);
  ASSERT_EQ(TestList()->RPop(), std::nullopt);
  ASSERT_EQ(TestList()->Size(), 0);
  ASSERT_EQ(TestList()->NodeCount(), 0);
  ASSERT_EQ(TestList()->Encoding(), List::Encoding::kListPack);
}

TEST_F(ListTest, RangeWithListPackEncoding) {
  ASSERT_TRUE(TestList()->RPush("one"));
  ASSERT_TRUE(TestList()->RPush("two"));
  ASSERT_TRUE(TestList()->RPush("three"));
  ASSERT_TRUE(TestList()->RPush("four"));

  ASSERT_EQ(TestList()->Range(0, 3),
            (std::vector<std::string>{"one", "two", "three", "four"}));
  ASSERT_EQ(TestList()->Range(1, 2),
            (std::vector<std::string>{"two", "three"}));
  ASSERT_EQ(TestList()->Range(2, 100),
            (std::vector<std::string>{"three", "four"}));
  ASSERT_TRUE(TestList()->Range(4, 5).empty());
  ASSERT_TRUE(TestList()->Range(2, 1).empty());
}

TEST(ListEncodingTest, ConvertsFromListPackToQuickListWhenListGrows) {
  auto list = List::Create(96);
  const std::string value(32, 'x');

  ASSERT_TRUE(list->RPush(value + "1"));
  ASSERT_TRUE(list->RPush(value + "2"));
  ASSERT_EQ(list->Encoding(), List::Encoding::kListPack);

  ASSERT_TRUE(list->RPush(value + "3"));

  ASSERT_EQ(list->Encoding(), List::Encoding::kQuickList);
  ASSERT_EQ(list->Size(), 3);
  ASSERT_EQ(list->NodeCount(), 2);
  ASSERT_EQ(list->LPop(), value + "1");
  ASSERT_EQ(list->LPop(), value + "2");
  ASSERT_EQ(list->LPop(), value + "3");
}

TEST(ListEncodingTest, ConvertsBackToListPackAfterShrinking) {
  auto list = List::Create(96);
  const std::string value(32, 'y');

  ASSERT_TRUE(list->RPush(value + "1"));
  ASSERT_TRUE(list->RPush(value + "2"));
  ASSERT_TRUE(list->RPush(value + "3"));
  ASSERT_EQ(list->Encoding(), List::Encoding::kQuickList);

  ASSERT_EQ(list->RPop(), value + "3");
  ASSERT_EQ(list->Encoding(), List::Encoding::kQuickList);
  ASSERT_EQ(list->RPop(), value + "2");

  ASSERT_EQ(list->Encoding(), List::Encoding::kListPack);
  ASSERT_EQ(list->Size(), 1);
  ASSERT_EQ(list->NodeCount(), 0);
  ASSERT_EQ(list->LPop(), value + "1");
}

TEST(ListEncodingTest, RangeWithQuickListEncoding) {
  auto list = List::Create(96);
  const std::string value(32, 'z');

  ASSERT_TRUE(list->RPush(value + "1"));
  ASSERT_TRUE(list->RPush(value + "2"));
  ASSERT_TRUE(list->RPush(value + "3"));
  ASSERT_TRUE(list->RPush(value + "4"));
  ASSERT_EQ(list->Encoding(), List::Encoding::kQuickList);

  ASSERT_EQ(list->Range(1, 3),
            (std::vector<std::string>{value + "2", value + "3", value + "4"}));
}
}  // namespace redis_simple::list
