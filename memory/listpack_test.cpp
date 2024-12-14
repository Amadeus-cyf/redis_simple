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
  std::string s0("test string 0");
  ASSERT_TRUE(listpack->Append(s0));
  ASSERT_EQ(listpack->GetNumOfElements(), 2);

  size_t idx = ListPack::ListPackHeaderSize;
  ASSERT_EQ(listpack->GetInteger(idx), -1234);
  size_t l0 = 0;
  unsigned char* c0 = listpack->Get(idx, &l0);
  ASSERT_TRUE(std::equal(c0, c0 + 5, "-1234"));

  size_t l1 = 0;
  idx = listpack->Next(idx);
  unsigned char* c1 = listpack->Get(idx, &l1);
  ASSERT_EQ(l1, s0.size());
  ASSERT_TRUE(std::equal(c1, c1 + l1, s0.c_str()));

  ASSERT_TRUE(listpack->AppendInteger(INT16_MAX >> 3));
  ASSERT_TRUE(listpack->AppendInteger(INT16_MAX));
  ASSERT_TRUE(listpack->AppendInteger(INT32_MAX >> 8));
  ASSERT_TRUE(listpack->AppendInteger(INT32_MAX));
  ASSERT_TRUE(listpack->AppendInteger(INT64_MAX));
  ASSERT_TRUE(listpack->Append("-1234567890"));

  std::string s2(4095, 'a');
  std::string s3(10000, 'c');
  ASSERT_TRUE(listpack->Append(s2));
  ASSERT_TRUE(listpack->Append(s3));

  /* Insert a string which exceeds the listpack size threshold */
  // std::string s4(UINT32_MAX, 'f');
  // ASSERT_FALSE(listpack->Append(s4));

  ASSERT_EQ(listpack->GetNumOfElements(), 10);
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
  ASSERT_TRUE(std::equal(c2, c2 + l2, "-1234567890"));
  ASSERT_EQ(listpack->GetInteger(idx), -1234567890);

  idx = listpack->Next(idx);
  size_t l3 = 0;
  unsigned char* c3 = listpack->Get(idx, &l3);
  ASSERT_EQ(l3, 4095);
  ASSERT_TRUE(std::equal(c3, c3 + l3, s2.c_str()));

  idx = listpack->Next(idx);
  size_t l4 = 0;
  unsigned char* c4 = listpack->Get(idx, &l4);
  ASSERT_EQ(l4, 10000);
  ASSERT_TRUE(std::equal(c4, c4 + l4, s3.c_str()));
  idx = listpack->Next(idx);

  /* Reach EOF */
  ASSERT_EQ(idx, listpack->GetTotalBytes() - 1);
  ASSERT_EQ(listpack->Next(idx), -1);

  /* Invalid get for string type */
  ASSERT_THROW(listpack->Get(ListPack::ListPackHeaderSize - 1, nullptr),
               std::out_of_range);
  ASSERT_THROW(listpack->Get(listpack->GetTotalBytes(), nullptr),
               std::out_of_range);

  /* Invalid get for integer type */
  ASSERT_THROW(listpack->GetInteger(3), std::out_of_range);
  ASSERT_THROW(listpack->GetInteger(listpack->GetTotalBytes()),
               std::out_of_range);
}

TEST_F(ListPackTest, Prepend) {
  std::string s0(4094, 'c');
  std::string s1(4096, 'e');
  std::string s2("test string 2");
  std::string s3("123456789");
  listpack->Prepend(s0);
  listpack->PrependInteger(INT64_MAX - 4);
  listpack->Prepend(s1);
  listpack->PrependInteger(INT16_MIN + 3);
  listpack->Prepend(s2);
  listpack->Prepend(s3);
  ASSERT_EQ(listpack->GetNumOfElements(), 16);

  size_t idx = ListPack::ListPackHeaderSize;
  size_t l0 = 0;
  unsigned char* c0 = listpack->Get(idx, &l0);
  ASSERT_EQ(l0, s3.size());
  ASSERT_TRUE(std::equal(c0, c0 + l0, "123456789"));
  ASSERT_EQ(listpack->GetInteger(idx), 123456789);

  idx = listpack->Next(idx);
  size_t l1 = 0;
  unsigned char* c1 = listpack->Get(idx, &l1);
  ASSERT_EQ(l1, s2.size());
  ASSERT_TRUE(std::equal(c1, c1 + l1, s2.c_str()));

  idx = listpack->Next(idx);
  size_t l2 = 0;
  unsigned char* c2 = listpack->Get(idx, &l2);
  ASSERT_EQ(l2, 6);
  ASSERT_TRUE(std::equal(c2, c2 + l2, "-32765"));
  ASSERT_EQ(listpack->GetInteger(idx), INT16_MIN + 3);

  idx = listpack->Next(idx);
  size_t l3 = 0;
  unsigned char* c3 = listpack->Get(idx, &l3);
  ASSERT_EQ(l3, s1.size());
  ASSERT_TRUE(std::equal(c3, c3 + l3, s1.c_str()));

  idx = listpack->Next(idx);
  size_t l4 = 0;
  unsigned char* c4 = listpack->Get(idx, &l4);
  ASSERT_EQ(l4, 19);
  ASSERT_TRUE(std::equal(c4, c4 + l4, "9223372036854775803"));
  ASSERT_EQ(listpack->GetInteger(idx), INT64_MAX - 4);

  idx = listpack->Next(idx);
  size_t l5 = 0;
  unsigned char* c5 = listpack->Get(idx, &l5);
  ASSERT_EQ(l5, s0.size());
  ASSERT_TRUE(std::equal(c5, c5 + l5, s0.c_str()));
}

TEST_F(ListPackTest, Insert) {
  std::string s0("test string 2");
  std::string s1(4094, 'f');
  std::string s2(4097, 'g');

  size_t idx = ListPack::ListPackHeaderSize;
  idx = listpack->Next(idx);
  idx = listpack->Next(idx);
  idx = listpack->Next(idx);
  size_t i0 = idx;
  ASSERT_TRUE(listpack->Insert(i0, s0));

  idx = listpack->Next(idx);
  idx = listpack->Next(idx);
  size_t i1 = idx;
  ASSERT_TRUE(listpack->InsertInteger(i1, INT64_MAX - 7));

  idx = listpack->Next(idx);
  idx = listpack->Next(idx);
  size_t i2 = idx;
  ASSERT_TRUE(listpack->InsertInteger(i2, INT32_MIN + 7));

  idx = listpack->Next(idx);
  size_t i3 = idx;
  ASSERT_TRUE(listpack->Insert(i3, s1));

  idx = listpack->Next(idx);
  idx = listpack->Next(idx);
  size_t i4 = idx;
  ASSERT_TRUE(listpack->Insert(i4, s2));

  /* Insert out of bound */
  ASSERT_FALSE(listpack->Insert(0, "test invalid insert"));
  ASSERT_FALSE(listpack->InsertInteger(listpack->GetTotalBytes(), 1));

  ASSERT_EQ(listpack->GetNumOfElements(), 21);

  size_t l0 = 0;
  unsigned char* c0 = listpack->Get(i0, &l0);
  ASSERT_EQ(l0, s0.size());
  ASSERT_TRUE(std::equal(c0, c0 + l0, s0.c_str()));

  size_t l1 = 0;
  unsigned char* c1 = listpack->Get(i1, &l1);
  ASSERT_EQ(l1, 19);
  ASSERT_TRUE(std::equal(c1, c1 + l1, "9223372036854775800"));
  ASSERT_EQ(listpack->GetInteger(i1), INT64_MAX - 7);

  size_t l2 = 0;
  unsigned char* c2 = listpack->Get(i2, &l2);
  ASSERT_EQ(l2, 11);
  ASSERT_TRUE(std::equal(c2, c2 + l2, "-2147483641"));
  ASSERT_EQ(listpack->GetInteger(i2), INT32_MIN + 7);

  size_t l3 = 0;
  unsigned char* c3 = listpack->Get(i3, &l3);
  ASSERT_EQ(l3, 4094);
  ASSERT_TRUE(std::equal(c3, c3 + l3, s1.c_str()));

  size_t l4 = 0;
  unsigned char* c4 = listpack->Get(i4, &l4);
  ASSERT_EQ(l4, 4097);
  ASSERT_TRUE(std::equal(c4, c4 + l4, s2.c_str()));
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
  ASSERT_EQ(listpack->GetNumOfElements(), 30);
  for (const ListPack::ListPackEntry& entry : entries) {
    size_t len = 0;
    unsigned char* c = listpack->Get(idx, &len);
    if (entry.str) {
      /* Type string */
      ASSERT_EQ(len, entry.str->size());
      ASSERT_TRUE(std::equal(c, c + len, entry.str->c_str()));
    } else {
      /* Type integer */
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

  /* Append an empty list */
  ASSERT_FALSE(listpack->BatchAppend({}));
}

TEST_F(ListPackTest, BatchPrepend) {
  std::string s0("test string 3");
  std::string s1(4094, 'h');
  std::string s2(4098, 'i');
  std::vector<ListPack::ListPackEntry> entries = {
      {
          .str = &s0,
      },
      {
          .sval = INT64_MIN + 7,
      },
      {
          .sval = INT64_MAX - 9,
      },
      {
          .sval = INT32_MIN + 7,
      },
      {
          .sval = INT32_MAX - 9,
      },
      {
          .sval = (INT32_MAX >> 8) - 1,
      },
      {
          .sval = 125,
      },
      {
          .str = &s1,
      },
      {
          .str = &s2,
      },
  };
  ASSERT_TRUE(listpack->BatchPrepend(entries));
  ASSERT_EQ(listpack->GetNumOfElements(), 39);
  size_t idx = ListPack::ListPackHeaderSize;
  for (const ListPack::ListPackEntry& entry : entries) {
    size_t len = 0;
    unsigned char* c = listpack->Get(idx, &len);
    if (entry.str) {
      /* Type String */
      ASSERT_EQ(len, entry.str->size());
      ASSERT_TRUE(std::equal(c, c + len, entry.str->c_str()));
    } else {
      /* Type integer */
      const std::string& sval_str = std::to_string(entry.sval);
      ASSERT_EQ(len, sval_str.size());
      ASSERT_TRUE(std::equal(c, c + len, sval_str.c_str()));
      ASSERT_EQ(listpack->GetInteger(idx), entry.sval);
    }
    idx = listpack->Next(idx);
  }

  /* Prepend an empty list */
  ASSERT_FALSE(listpack->BatchPrepend({}));
}

TEST_F(ListPackTest, BatchInsert) {
  std::string s0("test string 4");
  std::string s1(4093, 'j');
  std::string s2(4099, 'k');
  std::vector<ListPack::ListPackEntry> entries = {
      {
          .str = &s0,
      },
      {
          .sval = INT64_MIN + 21,
      },
      {
          .sval = INT64_MAX - 17,
      },
      {
          .sval = INT32_MIN + 21,
      },
      {
          .sval = INT32_MAX - 17,
      },
      {
          .sval = (INT32_MAX >> 8) - 17,
      },
      {
          .sval = INT16_MAX - 17,
      },
      {
          .sval = 123,
      },
      {
          .str = &s1,
      },
      {
          .str = &s2,
      },
  };
  size_t idx = ListPack::ListPackHeaderSize;
  idx = listpack->Next(idx);
  idx = listpack->Next(idx);
  idx = listpack->Next(idx);
  ASSERT_TRUE(listpack->BatchInsert(idx, entries));
  ASSERT_EQ(listpack->GetNumOfElements(), 49);
  for (const ListPack::ListPackEntry& entry : entries) {
    size_t len = 0;
    unsigned char* c = listpack->Get(idx, &len);
    if (entry.str) {
      /* Type string */
      ASSERT_EQ(len, entry.str->size());
      ASSERT_TRUE(std::equal(c, c + len, entry.str->c_str()));
    } else {
      /* Type integer */
      const std::string& sval_str = std::to_string(entry.sval);
      ASSERT_EQ(len, sval_str.size());
      ASSERT_TRUE(std::equal(c, c + len, sval_str.c_str()));
      ASSERT_EQ(listpack->GetInteger(idx), entry.sval);
    }
    idx = listpack->Next(idx);
  }

  /* Insert out of bound */
  ASSERT_FALSE(listpack->BatchInsert(0, entries));
  ASSERT_FALSE(listpack->BatchInsert(listpack->GetTotalBytes(), entries));

  /* Insert an empty list */
  ASSERT_FALSE(listpack->BatchInsert(0, {}));
}
}  // namespace in_memory
}  // namespace redis_simple
