#include "memory/listpack.h"

#include <gtest/gtest.h>

namespace redis_simple {
namespace in_memory {
class ListPackTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { listpack = new ListPack(0); }
  static void TearDownTestSuite() {
    delete listpack;
    listpack = nullptr;
  }
  static ListPack* listpack;
};

ListPack* ListPackTest::listpack = nullptr;

TEST_F(ListPackTest, Append) {
  ASSERT_TRUE(listpack->AppendInteger(-1234));
  const std::string& s1("test string 0\n");
  ASSERT_TRUE(listpack->Append(s1));
  ASSERT_EQ(listpack->GetNumOfElements(), 2);
  ASSERT_EQ(listpack->GetInteger(ListPack::ListPackHeaderSize), -1234);
  size_t l0 = 0;
  unsigned char* i0 = listpack->Get(ListPack::ListPackHeaderSize, &l0);
  ASSERT_EQ(l0, 5);
  ASSERT_EQ(std::string(reinterpret_cast<char*>(i0), l0), "-1234");
  size_t l1 = 0;
  unsigned char* c1 =
      listpack->Get(listpack->Next(ListPack::ListPackHeaderSize), &l1);
  ASSERT_EQ(l1, s1.size());
  ASSERT_EQ(std::string(reinterpret_cast<char*>(c1), l1), s1);
}
}  // namespace in_memory
}  // namespace redis_simple
