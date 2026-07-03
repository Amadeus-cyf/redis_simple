#include "server/db/db.h"

#include <gtest/gtest.h>

#include "utils/time_utils.h"

namespace redis_simple::db {
TEST(RedisDbTest, SetLookupAndDeleteKey) {
  auto redis_db = RedisDb::Create();

  ASSERT_EQ(redis_db->SetKey("key", RedisObject::CreateWithString("value"), 0),
            DbStatus::kOk);
  const auto* object = redis_db->LookupKey("key");
  ASSERT_NE(object, nullptr);
  EXPECT_EQ(object->Type(), RedisObject::ObjectType::kString);
  EXPECT_EQ(object->String(), "value");

  EXPECT_EQ(redis_db->DeleteKey("key"), DbStatus::kOk);
  EXPECT_EQ(redis_db->LookupKey("key"), nullptr);
  EXPECT_EQ(redis_db->DeleteKey("key"), DbStatus::kError);
}

TEST(RedisDbTest, ExpiredKeyIsRemovedOnLookup) {
  auto redis_db = RedisDb::Create();
  ASSERT_EQ(
      redis_db->SetKey("expired", RedisObject::CreateWithString("gone"), 1),
      DbStatus::kOk);

  EXPECT_EQ(redis_db->LookupKey("expired"), nullptr);
  EXPECT_EQ(redis_db->DeleteKey("expired"), DbStatus::kError);
}

TEST(RedisDbTest, ReplacingKeyCanClearOrKeepTtl) {
  auto redis_db = RedisDb::Create();
  const int64_t future = utils::NowInMilliseconds() + 60'000;

  ASSERT_EQ(
      redis_db->SetKey("key", RedisObject::CreateWithString("old"), future),
      DbStatus::kOk);
  EXPECT_EQ(redis_db->ExpiredPercentage(), 1.0);

  ASSERT_EQ(redis_db->SetKey("key", RedisObject::CreateWithString("new"), 0,
                             ToInt(SetKeyFlag::kKeepTtl)),
            DbStatus::kOk);
  EXPECT_EQ(redis_db->ExpiredPercentage(), 1.0);
  ASSERT_NE(redis_db->LookupKey("key"), nullptr);
  EXPECT_EQ(redis_db->LookupKey("key")->String(), "new");

  ASSERT_EQ(redis_db->SetKey("key", RedisObject::CreateWithString("fresh"), 0),
            DbStatus::kOk);
  EXPECT_EQ(redis_db->ExpiredPercentage(), 0.0);
}
}  // namespace redis_simple::db
