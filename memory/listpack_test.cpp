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
  std::string s1("test string 0");
  ASSERT_TRUE(listpack->Append(s1));
  ASSERT_EQ(listpack->GetNumOfElements(), 2);

  size_t idx = ListPack::ListPackHeaderSize;
  ASSERT_EQ(listpack->GetInteger(idx), -1234);

  size_t l0 = 0;
  unsigned char* c0 = listpack->Get(idx, &l0);
  ASSERT_TRUE(std::equal(c0, c0 + 5, "-1234"));

  size_t l1 = 0;
  idx = listpack->Next(idx);
  unsigned char* c1 = listpack->Get(idx, &l1);
  ASSERT_EQ(l1, s1.size());
  ASSERT_TRUE(std::equal(c1, c1 + l1, s1.c_str()));

  ASSERT_TRUE(listpack->AppendInteger(INT16_MAX >> 3));
  ASSERT_TRUE(listpack->AppendInteger(INT16_MAX));
  ASSERT_TRUE(listpack->AppendInteger(INT32_MAX >> 8));
  ASSERT_TRUE(listpack->AppendInteger(INT32_MAX));
  ASSERT_TRUE(listpack->AppendInteger(INT64_MAX));

  std::string s2(4095, 'a');
  std::string s3(10000, 'c');
  ASSERT_TRUE(listpack->Append(s2));
  ASSERT_TRUE(listpack->Append(s3));

  /* insert a string which exceeds the listpack size threshold */
  // std::string s4(UINT32_MAX, 'f');
  // ASSERT_FALSE(listpack->Append(s4));

  ASSERT_EQ(listpack->GetNumOfElements(), 9);
  idx = listpack->Next(idx);
  ASSERT_EQ(listpack->GetInteger(idx), INT16_MAX >> 3);
  idx = listpack->Next(idx);
  ASSERT_EQ(listpack->GetInteger(idx), INT16_MAX);
  idx = listpack->Next(idx);
  ASSERT_EQ(listpack->GetInteger(idx), INT32_MAX >> 8);
  idx = listpack->Next(idx);
  ASSERT_EQ(listpack->GetInteger(idx), INT32_MAX);
  idx = listpack->Next(idx);
  ASSERT_EQ(listpack->GetInteger(idx), INT64_MAX);
  idx = listpack->Next(idx);

  size_t l2 = 0;
  unsigned char* c2 = listpack->Get(idx, &l2);
  ASSERT_EQ(l2, 4095);
  ASSERT_TRUE(std::equal(c2, c2 + l2, s2.c_str()));
  idx = listpack->Next(idx);

  size_t l3 = 0;
  unsigned char* c3 = listpack->Get(idx, &l3);
  ASSERT_EQ(l3, 10000);
  ASSERT_TRUE(std::equal(c3, c3 + l3, s3.c_str()));
  idx = listpack->Next(idx);

  /* Reach EOF */
  ASSERT_EQ(idx, listpack->GetTotalBytes() - 1);
  ASSERT_EQ(listpack->Next(idx), -1);
}

TEST_F(ListPackTest, BatchAppend) {
  std::string s0("test string 1");
  std::string s1("hello world");
  std::string s2("-1234567");
  std::vector<ListPack::ListPackEntry> entries = {
      {
          .str = &s0,
      },
      {
          .sval = INT64_MIN,
      },
      {
          .sval = INT64_MAX,
      },
      {
          .sval = INT32_MIN,
      },
      {
          .sval = INT32_MAX,
      },
      {
          .sval = INT32_MAX >> 8,
      },
      {
          .sval = 127,
      },
      {
          .str = &s1,
      },
      {
          .str = &s2,
      },
  };
  ssize_t idx = listpack->GetTotalBytes() - 1;
  listpack->BatchAppend(entries);
  ASSERT_EQ(listpack->GetNumOfElements(), 18);

  for (const ListPack::ListPackEntry& entry : entries) {
    size_t len = 0;
    unsigned char* c = listpack->Get(idx, &len);
    if (entry.str) {
      /* string */
      ASSERT_EQ(len, entry.str->size());
      ASSERT_TRUE(std::equal(c, c + len, entry.str->c_str()));
    } else {
      /* integer */
      const std::string& sval_str = std::to_string(entry.sval);
      ASSERT_EQ(len, sval_str.size());
      ASSERT_TRUE(std::equal(c, c + len, sval_str.c_str()));
      ASSERT_EQ(listpack->GetInteger(idx), entry.sval);
    }
    idx = listpack->Next(idx);
  }

  /* Reach EOF */
  ASSERT_EQ(idx, listpack->GetTotalBytes() - 1);
  ASSERT_EQ(listpack->Next(idx), -1);
}
}  // namespace in_memory
}  // namespace redis_simple
