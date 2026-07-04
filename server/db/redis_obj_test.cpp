#include "server/db/redis_obj.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace redis_simple::db {
TEST(RedisObjectTest, StringObjectExposesStringValue) {
  const auto object = RedisObject::CreateWithString("value");

  EXPECT_EQ(object->Type(), RedisObject::ObjectType::kString);
  EXPECT_EQ(object->String(), "value");
  EXPECT_THROW(object->Set(), std::invalid_argument);
  EXPECT_THROW(object->List(), std::invalid_argument);
  EXPECT_THROW(object->ZSet(), std::invalid_argument);
  EXPECT_THROW(object->Hash(), std::invalid_argument);
}

TEST(RedisObjectTest, CollectionObjectsExposeTypedStorage) {
  const auto set_object = RedisObject::CreateWithSet(set::Set::Create());
  EXPECT_EQ(set_object->Type(), RedisObject::ObjectType::kSet);
  ASSERT_NE(set_object->Set(), nullptr);
  EXPECT_THROW(set_object->String(), std::invalid_argument);

  const auto list_object = RedisObject::CreateWithList(list::List::Create());
  EXPECT_EQ(list_object->Type(), RedisObject::ObjectType::kList);
  ASSERT_NE(list_object->List(), nullptr);
  EXPECT_THROW(list_object->Set(), std::invalid_argument);

  const auto zset_object = RedisObject::CreateWithZSet(zset::ZSet::Create());
  EXPECT_EQ(zset_object->Type(), RedisObject::ObjectType::kZSet);
  ASSERT_NE(zset_object->ZSet(), nullptr);
  EXPECT_THROW(zset_object->List(), std::invalid_argument);

  const auto hash_object = RedisObject::CreateWithHash(hash::Hash::Create());
  EXPECT_EQ(hash_object->Type(), RedisObject::ObjectType::kHash);
  ASSERT_NE(hash_object->Hash(), nullptr);
  EXPECT_THROW(hash_object->ZSet(), std::invalid_argument);
}
}  // namespace redis_simple::db
