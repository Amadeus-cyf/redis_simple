#include "server/aof.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "server/db/db.h"
#include "server/db/redis_obj.h"
#include "server/reply.h"
#include "utils/time_utils.h"

namespace redis_simple::aof {
namespace {
class TempFile {
 public:
  TempFile() {
    constexpr std::string_view kTemplate = "/tmp/redis_simple_aof_XXXXXX";
    std::array<char, kTemplate.size() + 1> path{};
    std::copy(kTemplate.begin(), kTemplate.end(), path.begin());
    const int fd = mkstemp(path.data());
    if (fd < 0) {
      throw std::runtime_error("failed to create temporary AOF");
    }
    close(fd);
    path_ = path.data();
  }
  TempFile(const TempFile&) = delete;
  TempFile& operator=(const TempFile&) = delete;
  TempFile(TempFile&&) = delete;
  TempFile& operator=(TempFile&&) = delete;
  ~TempFile() { unlink(path_.c_str()); }

  [[nodiscard]] const std::string& Path() const { return path_; }

  [[nodiscard]] bool Append(std::string_view data) const {
    const int fd = open(path_.c_str(), O_WRONLY | O_APPEND);
    if (fd < 0) {
      return false;
    }
    size_t written = 0;
    while (written < data.size()) {
      const ssize_t result =
          write(fd, data.data() + written, data.size() - written);
      if (result > 0) {
        written += static_cast<size_t>(result);
      } else if (result < 0 && errno == EINTR) {
        continue;
      } else {
        close(fd);
        return false;
      }
    }
    return close(fd) == 0;
  }

  [[nodiscard]] off_t Size() const {
    struct stat info {};
    return stat(path_.c_str(), &info) == 0 ? info.st_size : -1;
  }

 private:
  std::string path_;
};

std::string Encode(std::string_view command,
                   const std::vector<std::string_view>& args) {
  std::string encoded = reply::FromArrayHeader(args.size() + 1);
  reply::AppendBulkString(command, &encoded);
  for (std::string_view arg : args) {
    reply::AppendBulkString(arg, &encoded);
  }
  return encoded;
}

Options Always(const TempFile& file) {
  return {file.Path(), FsyncPolicy::kAlways};
}
}  // namespace

TEST(AofTest, ReplaysCanonicalSetAndExpiration) {
  TempFile file;
  auto source = db::RedisDb::Create();
  const int64_t expire = utils::NowInMilliseconds() + 60'000;
  ASSERT_EQ(
      source->SetKey("key", db::RedisObject::CreateWithString("value"), expire),
      db::DbStatus::kOk);

  auto writer = Aof::Open(Always(file), source.get());
  ASSERT_NE(writer, nullptr);
  const std::vector<std::string_view> args = {"key", "value", "PX", "60000"};
  ASSERT_TRUE(writer->Append("SET", args, source.get()));
  writer.reset();

  auto restored = db::RedisDb::Create();
  auto reader = Aof::Open(Always(file), restored.get());
  ASSERT_NE(reader, nullptr);
  const auto* object = restored->LookupKey("key");
  ASSERT_NE(object, nullptr);
  EXPECT_EQ(object->String(), "value");
  EXPECT_EQ(restored->Expiration("key"), expire);
}

TEST(AofTest, DoesNotResurrectExpiredKeysDuringReplay) {
  TempFile file;
  std::string commands = Encode("SET", {"key", "value"});
  commands.append(Encode("PEXPIREAT", {"key", "1"}));
  commands.append(Encode("APPEND", {"key", "suffix"}));
  ASSERT_TRUE(file.Append(commands));

  auto restored = db::RedisDb::Create();
  auto aof = Aof::Open(Always(file), restored.get());

  ASSERT_NE(aof, nullptr);
  EXPECT_EQ(restored->LookupKey("key"), nullptr);
}

TEST(AofTest, TruncatesIncompleteFinalCommand) {
  TempFile file;
  const std::string complete = Encode("SET", {"key", "value"});
  ASSERT_TRUE(file.Append(complete));
  ASSERT_TRUE(file.Append("*2\r\n$3\r\nDEL\r\n$4\r\npar"));
  ASSERT_GT(file.Size(), static_cast<off_t>(complete.size()));

  auto restored = db::RedisDb::Create();
  auto aof = Aof::Open(Always(file), restored.get());

  ASSERT_NE(aof, nullptr);
  EXPECT_EQ(file.Size(), static_cast<off_t>(complete.size()));
  ASSERT_NE(restored->LookupKey("key"), nullptr);
  EXPECT_EQ(restored->LookupKey("key")->String(), "value");
}

TEST(AofTest, RejectsCorruptInput) {
  TempFile file;
  ASSERT_TRUE(file.Append("not-resp\r\n"));
  auto database = db::RedisDb::Create();

  EXPECT_EQ(Aof::Open(Always(file), database.get()), nullptr);
}

TEST(AofTest, FlushesBackgroundWritesOnShutdown) {
  TempFile file;
  auto source = db::RedisDb::Create();
  ASSERT_EQ(
      source->SetKey("key", db::RedisObject::CreateWithString("value"), 0),
      db::DbStatus::kOk);
  Options options{file.Path(), FsyncPolicy::kNo};
  auto writer = Aof::Open(options, source.get());
  ASSERT_NE(writer, nullptr);
  const std::vector<std::string_view> args = {"key", "value"};
  ASSERT_TRUE(writer->Append("SET", args, source.get()));
  writer->WaitUntilIdle();
  EXPECT_TRUE(writer->Healthy());
  writer.reset();

  auto restored = db::RedisDb::Create();
  auto reader = Aof::Open(Always(file), restored.get());
  ASSERT_NE(reader, nullptr);
  ASSERT_NE(restored->LookupKey("key"), nullptr);
  EXPECT_EQ(restored->LookupKey("key")->String(), "value");
}

TEST(AofTest, RewritesAllObjectTypesAndPreservesConcurrentWrites) {
  TempFile file;
  auto source = db::RedisDb::Create();
  const int64_t expire = utils::NowInMilliseconds() + 60'000;
  ASSERT_EQ(source->SetKey("string", db::RedisObject::CreateWithString("old"),
                           expire),
            db::DbStatus::kOk);

  auto set_value = set::Set::Create();
  auto list_value = list::List::Create();
  auto zset_value = zset::ZSet::Create();
  auto hash_value = hash::Hash::Create();
  for (int index = 0; index < 65; ++index) {
    const std::string suffix = std::to_string(index);
    ASSERT_TRUE(set_value->Add("member" + suffix));
    ASSERT_TRUE(list_value->RPush("value" + suffix));
    ASSERT_TRUE(zset_value->InsertOrUpdate("member" + suffix, index));
    ASSERT_TRUE(hash_value->Set("field" + suffix, "value" + suffix));
  }
  ASSERT_EQ(source->SetKey(
                "set", db::RedisObject::CreateWithSet(std::move(set_value)), 0),
            db::DbStatus::kOk);

  ASSERT_EQ(
      source->SetKey("list",
                     db::RedisObject::CreateWithList(std::move(list_value)), 0),
      db::DbStatus::kOk);

  ASSERT_EQ(
      source->SetKey("zset",
                     db::RedisObject::CreateWithZSet(std::move(zset_value)), 0),
      db::DbStatus::kOk);

  ASSERT_EQ(
      source->SetKey("hash",
                     db::RedisObject::CreateWithHash(std::move(hash_value)), 0),
      db::DbStatus::kOk);

  auto writer = Aof::Open(Always(file), source.get());
  ASSERT_NE(writer, nullptr);
  ASSERT_EQ(writer->StartRewrite(source.get()), RewriteResult::kStarted);

  ASSERT_EQ(source->SetKey(
                "string", db::RedisObject::CreateWithString("updated"), expire),
            db::DbStatus::kOk);
  const std::vector<std::string_view> set_args = {"string", "updated"};
  ASSERT_TRUE(writer->Append("SET", set_args, source.get()));
  writer->WaitUntilRewriteIdle();
  ASSERT_TRUE(writer->Healthy());
  writer.reset();

  auto restored = db::RedisDb::Create();
  auto reader = Aof::Open(Always(file), restored.get());
  ASSERT_NE(reader, nullptr);
  ASSERT_NE(restored->LookupKey("string"), nullptr);
  EXPECT_EQ(restored->LookupKey("string")->String(), "updated");
  EXPECT_EQ(restored->Expiration("string"), expire);
  ASSERT_NE(restored->LookupKey("set"), nullptr);
  EXPECT_EQ(restored->LookupKey("set")->Set()->Size(), 65);
  EXPECT_TRUE(restored->LookupKey("set")->Set()->HasMember("member64"));
  ASSERT_NE(restored->LookupKey("list"), nullptr);
  EXPECT_EQ(restored->LookupKey("list")->List()->Size(), 65);
  EXPECT_EQ(restored->LookupKey("list")->List()->At(64), "value64");
  ASSERT_NE(restored->LookupKey("zset"), nullptr);
  EXPECT_EQ(restored->LookupKey("zset")->ZSet()->Size(), 65);
  EXPECT_EQ(restored->LookupKey("zset")->ZSet()->Score("member64"), 64.0);
  ASSERT_NE(restored->LookupKey("hash"), nullptr);
  EXPECT_EQ(restored->LookupKey("hash")->Hash()->Size(), 65);
  EXPECT_EQ(restored->LookupKey("hash")->Hash()->Get("field64"), "value64");
}

TEST(AofTest, RewriteCompactsCommandHistory) {
  TempFile file;
  auto source = db::RedisDb::Create();
  auto writer = Aof::Open(Always(file), source.get());
  ASSERT_NE(writer, nullptr);

  for (int value = 0; value < 128; ++value) {
    const std::string text = std::to_string(value);
    ASSERT_EQ(source->SetKey("key", db::RedisObject::CreateWithString(text), 0),
              db::DbStatus::kOk);
    const std::vector<std::string_view> args = {"key", text};
    ASSERT_TRUE(writer->Append("SET", args, source.get()));
  }
  const off_t original_size = file.Size();
  ASSERT_GT(original_size, 0);

  ASSERT_EQ(writer->StartRewrite(source.get()), RewriteResult::kStarted);
  writer->WaitUntilRewriteIdle();

  EXPECT_GT(original_size, file.Size());
  writer.reset();

  auto restored = db::RedisDb::Create();
  auto reader = Aof::Open(Always(file), restored.get());
  ASSERT_NE(reader, nullptr);
  ASSERT_NE(restored->LookupKey("key"), nullptr);
  EXPECT_EQ(restored->LookupKey("key")->String(), "127");
}
}  // namespace redis_simple::aof
