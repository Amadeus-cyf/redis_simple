#include "storage/zset/zset_listpack.h"

#include "gtest/gtest.h"
#include "storage/zset/zset_storage_test_util.h"

namespace redis_simple::zset {
TEST(ZSetListPackTest, Add) { zset_storage_test::TestAdd<ZSetListPack>(); }

TEST(ZSetListPackTest, Rank) { zset_storage_test::TestRank<ZSetListPack>(); }

TEST(ZSetListPackTest, Update) {
  zset_storage_test::TestUpdate<ZSetListPack>();
}

TEST(ZSetListPackTest, RangeByRank) {
  zset_storage_test::TestRangeByRank<ZSetListPack>();
}

TEST(ZSetListPackTest, RangeByScore) {
  zset_storage_test::TestRangeByScore<ZSetListPack>();
}

TEST(ZSetListPackTest, Count) { zset_storage_test::TestCount<ZSetListPack>(); }

TEST(ZSetListPackTest, Delete) {
  zset_storage_test::TestDelete<ZSetListPack>();
}
}  // namespace redis_simple::zset
