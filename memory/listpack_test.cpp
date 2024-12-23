#include "memory/listpack.h"

#include <gtest/gtest.h>

namespace redis_simple {
namespace in_memory {
class ListPackTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { listpack = new ListPack(); }
  static void TearDownTestSuite() {
    delete listpack;
    listpack = nullptr;
  }
  static ListPack* listpack;
};

ListPack* ListPackTest::listpack = nullptr;

TEST_F(ListPackTest, Append) {
  ASSERT_TRUE(listpack->Append(-1234));
  std::string s0("test string 0");
  ASSERT_TRUE(listpack->Append(s0));
  ASSERT_EQ(listpack->GetNumOfElements(), 2);
  size_t bytes = listpack->GetTotalBytes();

  size_t idx = listpack->First();
  ASSERT_EQ(idx, ListPack::ListPackHeaderSize);
  ASSERT_EQ(listpack->GetInteger(idx), -1234);
  size_t l0 = 0;
  unsigned char* c0 = listpack->Get(idx, &l0);
  ASSERT_TRUE(std::equal(c0, c0 + 5, "-1234"));

  size_t l1 = 0;
  idx = listpack->Next(idx);
  unsigned char* c1 = listpack->Get(idx, &l1);
  ASSERT_EQ(l1, s0.size());
  ASSERT_TRUE(std::equal(c1, c1 + l1, s0.c_str()));

  ASSERT_TRUE(listpack->Append(INT16_MAX >> 3));
  ASSERT_TRUE(listpack->Append(INT16_MAX));
  ASSERT_TRUE(listpack->Append(INT32_MAX >> 8));
  ASSERT_TRUE(listpack->Append(INT32_MAX));
  ASSERT_TRUE(listpack->Append(INT64_MAX));
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

  /* Reach the end of the listpack */
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
  listpack->Prepend(INT64_MAX - 4);
  listpack->Prepend(s1);
  listpack->Prepend(INT16_MIN + 3);
  listpack->Prepend(s2);
  listpack->Prepend(s3);
  ASSERT_EQ(listpack->GetNumOfElements(), 16);

  size_t idx = listpack->First();
  ASSERT_EQ(idx, ListPack::ListPackHeaderSize);
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

  size_t idx = listpack->First();
  ASSERT_EQ(idx, ListPack::ListPackHeaderSize);
  idx = listpack->Next(idx);
  idx = listpack->Next(idx);
  idx = listpack->Next(idx);
  size_t i0 = idx;
  ASSERT_TRUE(listpack->Insert(i0, s0));

  idx = listpack->Next(idx);
  idx = listpack->Next(idx);
  size_t i1 = idx;
  ASSERT_TRUE(listpack->Insert(i1, INT64_MAX - 7));

  idx = listpack->Next(idx);
  idx = listpack->Next(idx);
  size_t i2 = idx;
  ASSERT_TRUE(listpack->Insert(i2, INT32_MIN + 7));

  idx = listpack->Next(idx);
  size_t i3 = idx;
  ASSERT_TRUE(listpack->Insert(i3, s1));

  idx = listpack->Next(idx);
  idx = listpack->Next(idx);
  size_t i4 = idx;
  ASSERT_TRUE(listpack->Insert(i4, s2));

  /* Insert out of bound */
  ASSERT_THROW(listpack->Insert(0, "test invalid insert"), std::out_of_range);
  ASSERT_THROW(listpack->Insert(listpack->GetTotalBytes(), 123456),
               std::out_of_range);

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

TEST_F(ListPackTest, Replace) {
  /* Replace string */
  std::string s0("test string update");
  size_t idx = listpack->First();
  ASSERT_EQ(idx, ListPack::ListPackHeaderSize);
  for (int i = 0; i < 10; ++i) idx = listpack->Next(idx);
  size_t num_of_elements = listpack->GetNumOfElements();
  /* Get next element to check if it's not changed after the replace  */
  size_t l0_next;
  size_t next0 = listpack->Next(idx);
  unsigned char* c0_next = listpack->Get(next0, &l0_next);
  /* Replace and get the element */
  listpack->Replace(idx, s0);
  ASSERT_EQ(listpack->GetNumOfElements(), num_of_elements);
  size_t l1, l1_next;
  unsigned char* c1 = listpack->Get(idx, &l1);
  ASSERT_EQ(l1, s0.size());
  ASSERT_TRUE(std::equal(c1, c1 + l1, s0.c_str()));
  /* Get the next element and check if it is not changed */
  size_t next1 = listpack->Next(idx);
  unsigned char* c1_next = listpack->Get(next1, &l1_next);
  ASSERT_EQ(l1_next, l0_next);
  ASSERT_TRUE(std::equal(c1_next, c1_next + l1_next, c0_next));

  /* Replace integer */
  for (int i = 0; i < 7; ++i) idx = listpack->Next(idx);
  /* Get next element to check if it's not changed after the replace  */
  size_t l2_next;
  size_t next2 = listpack->Next(idx);
  unsigned char* c2_next = listpack->Get(next2, &l2_next);
  /* Replace and get the element */
  listpack->Replace(idx, 17);
  ASSERT_EQ(listpack->GetNumOfElements(), num_of_elements);
  size_t l3, l3_next;
  unsigned char* c3 = listpack->Get(idx, &l3);
  ASSERT_EQ(l3, 2);
  ASSERT_TRUE(std::equal(c3, c3 + l3, "17"));
  ASSERT_EQ(listpack->GetInteger(idx), 17);
  /* Get the next element and check if it is not changed */
  size_t next3 = listpack->Next(idx);
  unsigned char* c3_next = listpack->Get(next3, &l3_next);
  ASSERT_EQ(l3_next, l2_next);
  ASSERT_TRUE(std::equal(c3_next, c3_next + l3_next, c2_next));

  /* Replace the first element */
  idx = listpack->First();
  std::string s1("test string update 1");
  /* Get next element to check if it's not changed after the replace  */
  size_t l4_next;
  size_t next4 = listpack->Next(idx);
  unsigned char* c4_next = listpack->Get(next4, &l4_next);
  /* Replace and get the element */
  listpack->Replace(idx, s1);
  ASSERT_EQ(listpack->GetNumOfElements(), num_of_elements);
  size_t l5, l5_next;
  unsigned char* c5 = listpack->Get(idx, &l5);
  ASSERT_EQ(l5, s1.size());
  ASSERT_TRUE(std::equal(c5, c5 + l5, s1.c_str()));
  /* Get the next element and check if it is not changed */
  size_t next5 = listpack->Next(idx);
  unsigned char* c5_next = listpack->Get(next5, &l5_next);
  ASSERT_EQ(l5_next, l4_next);
  ASSERT_TRUE(std::equal(c5_next, c5_next + l5_next, c4_next));

  /* Replace the last element */
  idx = listpack->Last();
  ASSERT_EQ(listpack->Next(idx), -1);
  /* Replace and get the element */
  listpack->Replace(idx, 217);
  ASSERT_EQ(listpack->GetNumOfElements(), num_of_elements);
  size_t l7, l7_next;
  unsigned char* c7 = listpack->Get(idx, &l7);
  ASSERT_EQ(l7, 3);
  ASSERT_TRUE(std::equal(c7, c7 + l7, "217"));
  ASSERT_EQ(listpack->GetInteger(idx), 217);
  /* Check if the element is the last element */
  ASSERT_EQ(listpack->Next(idx), -1);

  /* Replace out of bound */
  ASSERT_THROW(listpack->Replace(0, "test replace out of bound"),
               std::out_of_range);
  ASSERT_THROW(listpack->Replace(listpack->GetTotalBytes(), 123456789),
               std::out_of_range);
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

  /* Reach the end the listpack */
  ASSERT_EQ(idx, -1);

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
  size_t idx = listpack->First();
  ASSERT_EQ(idx, ListPack::ListPackHeaderSize);
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
  size_t idx = listpack->First();
  ASSERT_EQ(idx, ListPack::ListPackHeaderSize);
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
  ASSERT_THROW(listpack->BatchInsert(0, entries), std::out_of_range);
  ASSERT_THROW(listpack->BatchInsert(listpack->GetTotalBytes(), entries),
               std::out_of_range);

  /* Insert an empty list */
  ASSERT_FALSE(listpack->BatchInsert(0, {}));
}

TEST_F(ListPackTest, InvalidGet) {
  ASSERT_THROW(listpack->Get(0, nullptr), std::out_of_range);
  ASSERT_THROW(listpack->Get(listpack->GetTotalBytes(), nullptr),
               std::out_of_range);
  ASSERT_THROW(listpack->GetInteger(0), std::out_of_range);
  ASSERT_THROW(listpack->GetInteger(listpack->GetTotalBytes()),
               std::out_of_range);
}

TEST_F(ListPackTest, Find) {
  ssize_t i0 = listpack->Find("test string 0");
  ASSERT_NE(i0, -1);
  ssize_t i1 = listpack->Find("-1234567");
  ASSERT_NE(i1, -1);
  ssize_t i2 = listpack->Find("string not exist");
  ASSERT_EQ(i2, -1);
  ssize_t i3 = listpack->Find("-12345465657");
  ASSERT_EQ(i3, -1);
}

TEST_F(ListPackTest, Delete) {
  /* Delete the head */
  size_t idx = listpack->First(), prev_idx = -1, next_idx = listpack->Next(idx);
  size_t num_of_elements = listpack->GetNumOfElements();
  size_t l0 = 0;
  unsigned char* c0 = listpack->Get(next_idx, &l0);
  listpack->Delete(idx);
  idx = listpack->First();
  size_t l1 = 0;
  unsigned char* c1 = listpack->Get(idx, &l1);
  ASSERT_EQ(listpack->GetNumOfElements(), num_of_elements - 1);
  ASSERT_EQ(l0, l1);
  ASSERT_TRUE(std::equal(c1, c1 + l1, c0));

  /* Delete an element in the mid */
  idx = listpack->Next(idx);
  prev_idx = listpack->Next(idx);
  idx = listpack->Next(prev_idx);
  next_idx = listpack->Next(idx);
  size_t l2 = 0;
  unsigned char* c2 = listpack->Get(next_idx, &l2);
  num_of_elements = listpack->GetNumOfElements();
  listpack->Delete(idx);
  ASSERT_EQ(listpack->GetNumOfElements(), num_of_elements - 1);
  idx = listpack->Next(prev_idx);
  size_t l3 = 0;
  unsigned char* c3 = listpack->Get(idx, &l3);
  ASSERT_EQ(l2, l3);
  ASSERT_TRUE(std::equal(c3, c3 + l3, c2));

  /* Delete the tail */
  idx = listpack->Last();
  ASSERT_EQ(listpack->Next(idx), -1);
  size_t l4 = 0;
  unsigned char* c4 = listpack->Get(next_idx, &l2);
  prev_idx = listpack->Prev(idx);
  num_of_elements = listpack->GetNumOfElements();
  listpack->Delete(idx);
  ASSERT_EQ(listpack->GetNumOfElements(), num_of_elements - 1);
  size_t l5 = 0;
  unsigned char* c5 = listpack->Get(next_idx, &l2);
  ASSERT_EQ(l4, l5);
  ASSERT_TRUE(std::equal(c5, c5 + l5, c4));

  /* Delete out of bound */
  ASSERT_THROW(listpack->Delete(ListPack::ListPackHeaderSize - 1),
               std::out_of_range);
  ASSERT_THROW(listpack->Delete(listpack->GetTotalBytes()), std::out_of_range);
}
}  // namespace in_memory
}  // namespace redis_simple
