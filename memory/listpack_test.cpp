#include "memory/listpack.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace redis_simple::in_memory {
namespace {
ListPack::ListPackEntry StrEntry(std::string* str) { return {str, 0}; }

ListPack::ListPackEntry IntEntry(int64_t val) { return {nullptr, val}; }

std::string StringAt(const ListPack& listpack, size_t idx) {
  size_t len = 0;
  const unsigned char* data = listpack.Get(idx, &len);
  return {reinterpret_cast<const char*>(data), len};
}

void ExpectEntryAt(const ListPack& listpack, size_t idx,
                   const ListPack::ListPackEntry& entry) {
  if (entry.str != nullptr) {
    EXPECT_EQ(StringAt(listpack, idx), *entry.str);
    return;
  }

  const auto expected = std::to_string(entry.sval);
  EXPECT_EQ(StringAt(listpack, idx), expected);
  EXPECT_EQ(listpack.IntegerAt(idx), entry.sval);
}

ssize_t ExpectEntriesFrom(const ListPack& listpack, size_t start_idx,
                          const std::vector<ListPack::ListPackEntry>& entries) {
  auto idx = static_cast<ssize_t>(start_idx);
  for (const auto& entry : entries) {
    if (idx == -1) {
      ADD_FAILURE() << "listpack ended before all entries were checked";
      return idx;
    }
    const auto current_idx = static_cast<size_t>(idx);
    ExpectEntryAt(listpack, current_idx, entry);
    idx = listpack.Next(current_idx);
  }
  return idx;
}

size_t Advance(const ListPack& listpack, size_t idx, size_t steps) {
  for (size_t step = 0; step < steps; ++step) {
    const auto next = listpack.Next(idx);
    if (next == -1) {
      ADD_FAILURE() << "listpack ended before the requested advance";
      return idx;
    }
    idx = static_cast<size_t>(next);
  }
  return idx;
}

std::vector<ListPack::ListPackEntry> MakeBatchAppendEntries(std::string* s0,
                                                            std::string* s1,
                                                            std::string* s2) {
  return {
      StrEntry(s0),        IntEntry(INT64_MIN), IntEntry(INT64_MAX),
      IntEntry(INT32_MIN), IntEntry(INT32_MAX), IntEntry(INT32_MAX >> 8),
      IntEntry(127),       StrEntry(s1),        StrEntry(s2),
  };
}

std::vector<ListPack::ListPackEntry> MakeBatchPrependEntries(std::string* s0,
                                                             std::string* s1,
                                                             std::string* s2) {
  return {
      StrEntry(s0),
      IntEntry(INT64_MIN + 7),
      IntEntry(INT64_MAX - 9),
      IntEntry(INT32_MIN + 7),
      IntEntry(INT32_MAX - 9),
      IntEntry((INT32_MAX >> 8) - 1),
      IntEntry(125),
      StrEntry(s1),
      StrEntry(s2),
  };
}

std::vector<ListPack::ListPackEntry> MakeBatchInsertEntries(std::string* s0,
                                                            std::string* s1,
                                                            std::string* s2) {
  return {
      StrEntry(s0),
      IntEntry(INT64_MIN + 21),
      IntEntry(INT64_MAX - 17),
      IntEntry(INT32_MIN + 21),
      IntEntry(INT32_MAX - 17),
      IntEntry((INT32_MAX >> 8) - 17),
      IntEntry(INT16_MAX - 17),
      IntEntry(123),
      StrEntry(s1),
      StrEntry(s2),
  };
}

std::unique_ptr<ListPack> MakeAppendedListPack() {
  auto listpack = std::make_unique<ListPack>();
  listpack->Append(-1234);
  std::string s0("test string 0");
  listpack->Append(s0);
  listpack->Append(INT16_MAX >> 3);
  listpack->Append(INT16_MAX);
  listpack->Append(INT32_MAX >> 8);
  listpack->Append(INT32_MAX);
  listpack->Append(INT64_MAX);
  listpack->Append("-1234567890");
  listpack->Append(std::string(4095, 'a'));
  listpack->Append(std::string(10000, 'c'));
  return listpack;
}

std::unique_ptr<ListPack> MakePrependedListPack() {
  auto listpack = MakeAppendedListPack();
  listpack->Prepend(std::string(4094, 'c'));
  listpack->Prepend(INT64_MAX - 4);
  listpack->Prepend(std::string(4096, 'e'));
  listpack->Prepend(INT16_MIN + 3);
  listpack->Prepend(std::string("test string 2"));
  listpack->Prepend(std::string("123456789"));
  return listpack;
}

std::unique_ptr<ListPack> MakeInsertedListPack() {
  auto listpack = MakePrependedListPack();
  std::string s0("test string 2");
  std::string s1(4094, 'f');
  std::string s2(4097, 'g');
  size_t idx = Advance(*listpack, listpack->First(), 3);
  listpack->Insert(idx, s0);
  idx = Advance(*listpack, idx, 2);
  listpack->Insert(idx, INT64_MAX - 7);
  idx = Advance(*listpack, idx, 2);
  listpack->Insert(idx, INT32_MIN + 7);
  idx = Advance(*listpack, idx, 1);
  listpack->Insert(idx, s1);
  idx = Advance(*listpack, idx, 2);
  listpack->Insert(idx, s2);
  return listpack;
}

std::unique_ptr<ListPack> MakeReplacedListPack() {
  auto listpack = MakeInsertedListPack();
  size_t idx = listpack->First();
  for (int i = 0; i < 10; ++i) {
    idx = listpack->Next(idx);
  }
  listpack->Replace(idx, std::string("test string update"));
  for (int i = 0; i < 7; ++i) {
    idx = listpack->Next(idx);
  }
  listpack->Replace(idx, 17);
  listpack->Replace(listpack->First(), std::string("test string update 1"));
  listpack->Replace(listpack->Last(), 217);
  return listpack;
}

std::unique_ptr<ListPack> MakeBatchAppendedListPack() {
  auto listpack = MakeReplacedListPack();
  std::string s0("test string 1");
  std::string s1("hello world");
  std::string s2("-1234567");
  const auto entries = MakeBatchAppendEntries(&s0, &s1, &s2);
  listpack->BatchAppend(entries);
  return listpack;
}

std::unique_ptr<ListPack> MakeBatchPrependedListPack() {
  auto listpack = MakeBatchAppendedListPack();
  std::string s0("test string 3");
  std::string s1(4094, 'h');
  std::string s2(4098, 'i');
  const auto entries = MakeBatchPrependEntries(&s0, &s1, &s2);
  listpack->BatchPrepend(entries);
  return listpack;
}

std::unique_ptr<ListPack> MakeBatchInsertedListPack() {
  auto listpack = MakeBatchPrependedListPack();
  std::string s0("test string 4");
  std::string s1(4093, 'j');
  std::string s2(4099, 'k');
  const auto entries = MakeBatchInsertEntries(&s0, &s1, &s2);
  size_t idx = Advance(*listpack, listpack->First(), 3);
  listpack->BatchInsert(idx, entries);
  return listpack;
}
}  // namespace

TEST(ListPackTest, Append) {
  auto listpack = std::make_unique<ListPack>();
  ASSERT_EQ(listpack->First(), -1);
  ASSERT_EQ(listpack->Last(), -1);

  ASSERT_TRUE(listpack->Append(-1234));
  std::string s0("test string 0");
  ASSERT_TRUE(listpack->Append(s0));
  ASSERT_EQ(listpack->Size(), 2);

  size_t idx = listpack->First();
  ASSERT_EQ(idx, ListPack::kListPackHeaderSize);
  ASSERT_EQ(listpack->IntegerAt(idx), -1234);
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

  // Insert a string which exceeds the listpack size threshold.
  // std::string s4(UINT32_MAX, 'f');
  // ASSERT_FALSE(listpack->Append(s4));

  ASSERT_EQ(listpack->Size(), 10);
  idx = listpack->Next(idx);
  ASSERT_EQ(listpack->IntegerAt(idx), INT16_MAX >> 3);
  idx = listpack->Next(idx);
  ASSERT_EQ(listpack->IntegerAt(idx), INT16_MAX);
  idx = listpack->Next(idx);
  ASSERT_EQ(listpack->IntegerAt(idx), INT32_MAX >> 8);
  idx = listpack->Next(idx);
  ASSERT_EQ(listpack->IntegerAt(idx), INT32_MAX);
  idx = listpack->Next(idx);
  ASSERT_EQ(listpack->IntegerAt(idx), INT64_MAX);

  idx = listpack->Next(idx);
  size_t l2 = 0;
  unsigned char* c2 = listpack->Get(idx, &l2);
  ASSERT_TRUE(std::equal(c2, c2 + l2, "-1234567890"));
  ASSERT_EQ(listpack->IntegerAt(idx), -1234567890);

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

  // Reach the end of the listpack.
  ASSERT_EQ(listpack->Next(idx), -1);

  // Invalid get for string type.
  ASSERT_THROW(listpack->Get(ListPack::kListPackHeaderSize - 1, nullptr),
               std::out_of_range);
  ASSERT_THROW(listpack->Get(listpack->TotalBytes(), nullptr),
               std::out_of_range);

  // Invalid get for integer type.
  ASSERT_THROW(listpack->IntegerAt(3), std::out_of_range);
  ASSERT_THROW(listpack->IntegerAt(listpack->TotalBytes()), std::out_of_range);
}

TEST(ListPackTest, Prepend) {
  auto listpack = MakeAppendedListPack();
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
  ASSERT_EQ(listpack->Size(), 16);

  size_t idx = listpack->First();
  ASSERT_EQ(idx, ListPack::kListPackHeaderSize);
  size_t l0 = 0;
  unsigned char* c0 = listpack->Get(idx, &l0);
  ASSERT_EQ(l0, s3.size());
  ASSERT_TRUE(std::equal(c0, c0 + l0, "123456789"));
  ASSERT_EQ(listpack->IntegerAt(idx), 123456789);

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
  ASSERT_EQ(listpack->IntegerAt(idx), INT16_MIN + 3);

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
  ASSERT_EQ(listpack->IntegerAt(idx), INT64_MAX - 4);

  idx = listpack->Next(idx);
  size_t l5 = 0;
  unsigned char* c5 = listpack->Get(idx, &l5);
  ASSERT_EQ(l5, s0.size());
  ASSERT_TRUE(std::equal(c5, c5 + l5, s0.c_str()));
}

TEST(ListPackTest, Insert) {
  auto listpack = MakePrependedListPack();
  std::string s0("test string 2");
  std::string s1(4094, 'f');
  std::string s2(4097, 'g');

  size_t idx = listpack->First();
  ASSERT_EQ(idx, ListPack::kListPackHeaderSize);
  idx = Advance(*listpack, idx, 3);
  size_t i0 = idx;
  ASSERT_TRUE(listpack->Insert(i0, s0));

  idx = Advance(*listpack, idx, 2);
  size_t i1 = idx;
  ASSERT_TRUE(listpack->Insert(i1, INT64_MAX - 7));

  idx = Advance(*listpack, idx, 2);
  size_t i2 = idx;
  ASSERT_TRUE(listpack->Insert(i2, INT32_MIN + 7));

  idx = Advance(*listpack, idx, 1);
  size_t i3 = idx;
  ASSERT_TRUE(listpack->Insert(i3, s1));

  idx = Advance(*listpack, idx, 2);
  size_t i4 = idx;
  ASSERT_TRUE(listpack->Insert(i4, s2));

  // Insert out of bound.
  ASSERT_THROW(listpack->Insert(0, "test invalid insert"), std::out_of_range);
  ASSERT_THROW(listpack->Insert(listpack->TotalBytes(), 123456),
               std::out_of_range);

  ASSERT_EQ(listpack->Size(), 21);

  size_t l0 = 0;
  unsigned char* c0 = listpack->Get(i0, &l0);
  ASSERT_EQ(l0, s0.size());
  ASSERT_TRUE(std::equal(c0, c0 + l0, s0.c_str()));

  size_t l1 = 0;
  unsigned char* c1 = listpack->Get(i1, &l1);
  ASSERT_EQ(l1, 19);
  ASSERT_TRUE(std::equal(c1, c1 + l1, "9223372036854775800"));
  ASSERT_EQ(listpack->IntegerAt(i1), INT64_MAX - 7);

  size_t l2 = 0;
  unsigned char* c2 = listpack->Get(i2, &l2);
  ASSERT_EQ(l2, 11);
  ASSERT_TRUE(std::equal(c2, c2 + l2, "-2147483641"));
  ASSERT_EQ(listpack->IntegerAt(i2), INT32_MIN + 7);

  size_t l3 = 0;
  unsigned char* c3 = listpack->Get(i3, &l3);
  ASSERT_EQ(l3, 4094);
  ASSERT_TRUE(std::equal(c3, c3 + l3, s1.c_str()));

  size_t l4 = 0;
  unsigned char* c4 = listpack->Get(i4, &l4);
  ASSERT_EQ(l4, 4097);
  ASSERT_TRUE(std::equal(c4, c4 + l4, s2.c_str()));
}

TEST(ListPackTest, Replace) {
  auto listpack = MakeInsertedListPack();
  // Replace string.
  std::string s0("test string update");
  size_t idx = listpack->First();
  ASSERT_EQ(idx, ListPack::kListPackHeaderSize);
  for (int i = 0; i < 10; ++i) {
    idx = listpack->Next(idx);
  }
  size_t num_of_elements = listpack->Size();
  // Get next element to check if it's not changed after the replace.
  size_t l0_next = 0;
  size_t next0 = listpack->Next(idx);
  unsigned char* c0_next = listpack->Get(next0, &l0_next);
  std::string s0_next(reinterpret_cast<const char*>(c0_next), l0_next);
  // Replace and get the element.
  listpack->Replace(idx, s0);
  ASSERT_EQ(listpack->Size(), num_of_elements);
  size_t l1 = 0;
  size_t l1_next = 0;
  unsigned char* c1 = listpack->Get(idx, &l1);
  ASSERT_EQ(l1, s0.size());
  ASSERT_TRUE(std::equal(c1, c1 + l1, s0.c_str()));
  // Get the next element and check if it is not changed.
  size_t next1 = listpack->Next(idx);
  unsigned char* c1_next = listpack->Get(next1, &l1_next);
  ASSERT_EQ(l1_next, l0_next);
  ASSERT_TRUE(std::equal(c1_next, c1_next + l1_next, s0_next.c_str()));

  // Replace integer.
  for (int i = 0; i < 7; ++i) {
    idx = listpack->Next(idx);
  }
  // Get next element to check if it's not changed after the replace.
  size_t l2_next = 0;
  size_t next2 = listpack->Next(idx);
  unsigned char* c2_next = listpack->Get(next2, &l2_next);
  std::string s2_next(reinterpret_cast<const char*>(c2_next), l2_next);
  // Replace and get the element.
  listpack->Replace(idx, 17);
  ASSERT_EQ(listpack->Size(), num_of_elements);
  size_t l3 = 0;
  size_t l3_next = 0;
  unsigned char* c3 = listpack->Get(idx, &l3);
  ASSERT_EQ(l3, 2);
  ASSERT_TRUE(std::equal(c3, c3 + l3, "17"));
  ASSERT_EQ(listpack->IntegerAt(idx), 17);
  // Get the next element and check if it is not changed.
  size_t next3 = listpack->Next(idx);
  unsigned char* c3_next = listpack->Get(next3, &l3_next);
  ASSERT_EQ(l3_next, l2_next);
  ASSERT_TRUE(std::equal(c3_next, c3_next + l3_next, s2_next.c_str()));

  // Replace the first element.
  idx = listpack->First();
  std::string s1("test string update 1");
  // Get next element to check if it's not changed after the replace.
  size_t l4_next = 0;
  size_t next4 = listpack->Next(idx);
  unsigned char* c4_next = listpack->Get(next4, &l4_next);
  std::string s4_next(reinterpret_cast<const char*>(c4_next), l4_next);
  // Replace and get the element.
  listpack->Replace(idx, s1);
  ASSERT_EQ(listpack->Size(), num_of_elements);
  size_t l5 = 0;
  size_t l5_next = 0;
  unsigned char* c5 = listpack->Get(idx, &l5);
  ASSERT_EQ(l5, s1.size());
  ASSERT_TRUE(std::equal(c5, c5 + l5, s1.c_str()));
  // Get the next element and check if it is not changed.
  size_t next5 = listpack->Next(idx);
  unsigned char* c5_next = listpack->Get(next5, &l5_next);
  ASSERT_EQ(l5_next, l4_next);
  ASSERT_TRUE(std::equal(c5_next, c5_next + l5_next, s4_next.c_str()));

  // Replace the last element.
  idx = listpack->Last();
  ASSERT_EQ(listpack->Next(idx), -1);
  // Replace and get the element.
  listpack->Replace(idx, 217);
  ASSERT_EQ(listpack->Size(), num_of_elements);
  size_t l7 = 0;
  [[maybe_unused]] size_t l7_next = 0;
  unsigned char* c7 = listpack->Get(idx, &l7);
  ASSERT_EQ(l7, 3);
  ASSERT_TRUE(std::equal(c7, c7 + l7, "217"));
  ASSERT_EQ(listpack->IntegerAt(idx), 217);
  // Check if the element is the last element.
  ASSERT_EQ(listpack->Next(idx), -1);

  // Replace out of bound.
  ASSERT_THROW(listpack->Replace(0, "test replace out of bound"),
               std::out_of_range);
  ASSERT_THROW(listpack->Replace(listpack->TotalBytes(), 123456789),
               std::out_of_range);
}

TEST(ListPackTest, BatchAppend) {
  auto listpack = MakeReplacedListPack();
  std::string s0("test string 1");
  std::string s1("hello world");
  std::string s2("-1234567");
  const auto entries = MakeBatchAppendEntries(&s0, &s1, &s2);
  size_t idx = listpack->TotalBytes() - 1;
  listpack->BatchAppend(entries);
  ASSERT_EQ(listpack->Size(), 30);
  const auto end_idx = ExpectEntriesFrom(*listpack, idx, entries);
  ASSERT_EQ(end_idx, -1);

  // Append an empty list.
  ASSERT_FALSE(listpack->BatchAppend({}));
}

TEST(ListPackTest, BatchPrepend) {
  auto listpack = MakeBatchAppendedListPack();
  std::string s0("test string 3");
  std::string s1(4094, 'h');
  std::string s2(4098, 'i');
  const auto entries = MakeBatchPrependEntries(&s0, &s1, &s2);
  ASSERT_TRUE(listpack->BatchPrepend(entries));
  ASSERT_EQ(listpack->Size(), 39);
  size_t idx = listpack->First();
  ASSERT_EQ(idx, ListPack::kListPackHeaderSize);
  ExpectEntriesFrom(*listpack, idx, entries);

  // Prepend an empty list.
  ASSERT_FALSE(listpack->BatchPrepend({}));
}

TEST(ListPackTest, BatchInsert) {
  auto listpack = MakeBatchPrependedListPack();
  std::string s0("test string 4");
  std::string s1(4093, 'j');
  std::string s2(4099, 'k');
  const auto entries = MakeBatchInsertEntries(&s0, &s1, &s2);
  size_t idx = listpack->First();
  ASSERT_EQ(idx, ListPack::kListPackHeaderSize);
  idx = Advance(*listpack, idx, 3);
  ASSERT_TRUE(listpack->BatchInsert(idx, entries));
  ASSERT_EQ(listpack->Size(), 49);
  ExpectEntriesFrom(*listpack, idx, entries);

  // Insert out of bound.
  ASSERT_THROW(listpack->BatchInsert(0, entries), std::out_of_range);
  ASSERT_THROW(listpack->BatchInsert(listpack->TotalBytes(), entries),
               std::out_of_range);

  // Insert an empty list.
  ASSERT_FALSE(listpack->BatchInsert(0, {}));
}

TEST(ListPackTest, InvalidGet) {
  auto listpack = std::make_unique<ListPack>();
  // Out of bound
  ASSERT_THROW(listpack->Get(0, nullptr), std::out_of_range);
  ASSERT_THROW(listpack->Get(listpack->TotalBytes(), nullptr),
               std::out_of_range);
  ASSERT_THROW(listpack->IntegerAt(0), std::out_of_range);
  ASSERT_THROW(listpack->IntegerAt(listpack->TotalBytes()), std::out_of_range);
}

TEST(ListPackTest, Find) {
  auto listpack = MakeBatchInsertedListPack();
  ssize_t i0 = listpack->Find("test string 0");
  ASSERT_NE(i0, -1);
  ssize_t i1 = listpack->Find("-1234567");
  ASSERT_NE(i1, -1);
  ssize_t i2 = listpack->Find("string not exist");
  ASSERT_EQ(i2, -1);
  ssize_t i3 = listpack->Find("-12345465657");
  ASSERT_EQ(i3, -1);
}

TEST(ListPackTest, Iterate) {
  auto listpack = MakeBatchInsertedListPack();
  ssize_t idx = listpack->First();
  ASSERT_EQ(idx, ListPack::kListPackHeaderSize);

  ssize_t prev = -1;
  size_t len = 0;
  while (idx != -1) {
    ++len;
    prev = idx;
    idx = listpack->Next(idx);
  }
  ASSERT_EQ(prev, listpack->Last());
  ASSERT_EQ(len, listpack->Size());
}

TEST(ListPackTest, Delete) {
  auto listpack = MakeBatchInsertedListPack();
  // Delete the head.
  size_t idx = listpack->First();
  size_t prev_idx = -1;
  size_t next_idx = listpack->Next(idx);
  size_t num_of_elements = listpack->Size();
  size_t l0 = 0;
  unsigned char* c0 = listpack->Get(next_idx, &l0);
  listpack->Delete(idx);
  idx = listpack->First();
  size_t l1 = 0;
  unsigned char* c1 = listpack->Get(idx, &l1);
  ASSERT_EQ(listpack->Size(), num_of_elements - 1);
  ASSERT_EQ(l0, l1);
  ASSERT_TRUE(std::equal(c1, c1 + l1, c0));

  // Delete an element in the mid.
  idx = listpack->Next(idx);
  prev_idx = listpack->Next(idx);
  idx = listpack->Next(prev_idx);
  next_idx = listpack->Next(idx);
  size_t l2 = 0;
  unsigned char* c2 = listpack->Get(next_idx, &l2);
  num_of_elements = listpack->Size();
  listpack->Delete(idx);
  ASSERT_EQ(listpack->Size(), num_of_elements - 1);
  idx = listpack->Next(prev_idx);
  size_t l3 = 0;
  unsigned char* c3 = listpack->Get(idx, &l3);
  ASSERT_EQ(l2, l3);
  ASSERT_TRUE(std::equal(c3, c3 + l3, c2));

  // Delete the tail.
  idx = listpack->Last();
  ASSERT_EQ(listpack->Next(idx), -1);
  size_t l4 = 0;
  unsigned char* c4 = listpack->Get(next_idx, &l4);
  num_of_elements = listpack->Size();
  listpack->Delete(idx);
  ASSERT_EQ(listpack->Size(), num_of_elements - 1);
  size_t l5 = 0;
  unsigned char* c5 = listpack->Get(next_idx, &l5);
  ASSERT_EQ(l4, l5);
  ASSERT_TRUE(std::equal(c5, c5 + l5, c4));

  // Delete out of bound.
  ASSERT_THROW(listpack->Delete(ListPack::kListPackHeaderSize - 1),
               std::out_of_range);
  ASSERT_THROW(listpack->Delete(listpack->TotalBytes()), std::out_of_range);
}
}  // namespace redis_simple::in_memory
