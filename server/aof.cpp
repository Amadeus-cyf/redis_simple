#include "server/aof.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "logging/logger.h"
#include "server/client.h"
#include "server/commands/command.h"
#include "server/db/db.h"
#include "server/reply.h"
#include "server/request_parser.h"
#include "utils/float_utils.h"

namespace redis_simple::aof {
namespace {
constexpr size_t kReadChunkSize = size_t{64} * 1024;
constexpr size_t kMaxReplayCommandBytes = size_t{64} * 1024 * 1024;
constexpr size_t kSnapshotBatchSize = 64;
// Covers a bulk-string frame or array header, including a size_t decimal value.
constexpr size_t kRespElementOverhead = 32;
constexpr std::string_view kLongestInt64 = "-9223372036854775808";
constexpr auto kSyncInterval = std::chrono::seconds(1);

size_t MaxIovCount() {
  static const size_t max_iov_count = [] {
    const long system_limit = sysconf(_SC_IOV_MAX);
    const size_t limit =
        system_limit > 0 ? static_cast<size_t>(system_limit) : 1024;
    return std::min(limit,
                    static_cast<size_t>(std::numeric_limits<int>::max()));
  }();
  return max_iov_count;
}

bool WriteAll(int fd, std::string_view data) {
  size_t written = 0;
  while (written < data.size()) {
    const ssize_t result =
        write(fd, data.data() + written, data.size() - written);
    if (result > 0) {
      written += static_cast<size_t>(result);
      continue;
    }
    if (result < 0 && errno == EINTR) {
      continue;
    }
    return false;
  }
  return true;
}

bool SyncFile(int fd) {
  while (fsync(fd) < 0) {
    if (errno != EINTR) {
      return false;
    }
  }
  return true;
}

bool AddSize(size_t value, size_t* const total) {
  if (value > std::numeric_limits<size_t>::max() - *total) {
    return false;
  }
  *total += value;
  return true;
}

bool ReserveResp(size_t payload_size, size_t element_count,
                 size_t command_count, std::string* const output) {
  const size_t max_size = std::numeric_limits<size_t>::max();
  if (command_count > max_size - element_count) {
    return false;
  }
  const size_t overhead_count = element_count + command_count;
  if (overhead_count > max_size / kRespElementOverhead) {
    return false;
  }
  const size_t overhead = kRespElementOverhead * overhead_count;
  size_t reserve_size = output->size();
  if (!AddSize(payload_size, &reserve_size) ||
      !AddSize(overhead, &reserve_size) || reserve_size > output->max_size()) {
    return false;
  }
  if (reserve_size > output->capacity()) {
    const size_t capacity = output->capacity();
    const size_t grown_capacity =
        capacity <= output->max_size() / 2 ? capacity * 2 : output->max_size();
    output->reserve(std::max(reserve_size, grown_capacity));
  }
  return true;
}

bool ReserveCommands(std::initializer_list<std::string_view> elements,
                     size_t command_count, std::string* const output) {
  size_t payload_size = 0;
  for (const auto element : elements) {
    if (!AddSize(element.size(), &payload_size)) {
      return false;
    }
  }
  return ReserveResp(payload_size, elements.size(), command_count, output);
}

bool ReserveCommand(std::string_view command,
                    const std::vector<std::string_view>& args,
                    std::string* const output) {
  size_t payload_size = command.size();
  for (const auto arg : args) {
    if (!AddSize(arg.size(), &payload_size)) {
      return false;
    }
  }

  if (args.size() == std::numeric_limits<size_t>::max()) {
    return false;
  }
  return ReserveResp(payload_size, args.size() + 1, 1, output);
}

bool ReserveSetRecord(std::string_view key, std::string_view value,
                      bool has_expiration, std::string* const output) {
  if (!has_expiration) {
    return ReserveCommands({"SET", key, value}, 1, output);
  }
  return ReserveCommands({"SET", key, value, "PEXPIREAT", key, kLongestInt64},
                         2, output);
}

bool AppendCommand(std::string_view command,
                   const std::vector<std::string_view>& args,
                   std::string* const output) {
  if (!ReserveCommand(command, args, output)) {
    return false;
  }
  reply::AppendArrayHeader(args.size() + 1, output);
  reply::AppendBulkString(command, output);
  for (std::string_view arg : args) {
    reply::AppendBulkString(arg, output);
  }
  return true;
}

bool AppendFixedCommand(std::initializer_list<std::string_view> elements,
                        std::string* const output) {
  if (elements.size() == 0 || !ReserveCommands(elements, 1, output)) {
    return false;
  }
  reply::AppendArrayHeader(elements.size(), output);
  for (const auto element : elements) {
    reply::AppendBulkString(element, output);
  }
  return true;
}

bool BeginSnapshotBatch(std::string_view command, std::string_view key,
                        size_t item_index, size_t item_count,
                        size_t elements_per_item, std::string* const output) {
  if (item_index % kSnapshotBatchSize != 0) {
    return true;
  }
  const size_t batch_size =
      std::min(kSnapshotBatchSize, item_count - item_index);
  const size_t element_count = 2 + (batch_size * elements_per_item);
  if (!ReserveResp(command.size() + key.size(), element_count, 1, output)) {
    return false;
  }
  reply::AppendArrayHeader(element_count, output);
  reply::AppendBulkString(command, output);
  reply::AppendBulkString(key, output);
  return true;
}

bool AppendSingleKeyCommand(std::string_view command, std::string_view key,
                            std::string* const output) {
  if (!ReserveCommands({command, key}, 1, output)) {
    return false;
  }
  reply::AppendArrayHeader(2, output);
  reply::AppendBulkString(command, output);
  reply::AppendBulkString(key, output);
  return true;
}

bool AppendExpireAt(std::string_view key, int64_t expire, std::string* output) {
  std::array<char, std::numeric_limits<int64_t>::digits10 + 3> buffer{};
  const auto encoded =
      std::to_chars(buffer.data(), buffer.data() + buffer.size(), expire);
  if (encoded.ec != std::errc()) {
    return false;
  }
  const std::string_view encoded_expire(
      buffer.data(), static_cast<size_t>(encoded.ptr - buffer.data()));
  if (!ReserveCommands({"PEXPIREAT", key, encoded_expire}, 1, output)) {
    return false;
  }
  reply::AppendArrayHeader(3, output);
  reply::AppendBulkString("PEXPIREAT", output);
  reply::AppendBulkString(key, output);
  reply::AppendBulkString(encoded_expire, output);
  return true;
}

bool WriteBatch(int fd, std::deque<std::string>& batch,
                std::vector<iovec>* const blocks) {
  const size_t max_iov_count = MaxIovCount();
  blocks->reserve(std::min(batch.size(), max_iov_count));
  auto command = batch.begin();

  while (command != batch.end()) {
    blocks->clear();
    for (; command != batch.end() && blocks->size() < max_iov_count;
         ++command) {
      blocks->push_back({command->data(), command->size()});
    }

    size_t index = 0;
    while (index < blocks->size()) {
      ssize_t written = -1;
      while (true) {
        written = writev(fd, blocks->data() + index,
                         static_cast<int>(blocks->size() - index));
        if (written >= 0 || errno != EINTR) {
          break;
        }
      }
      if (written <= 0) {
        return false;
      }
      auto consumed = static_cast<size_t>(written);
      while (consumed > 0) {
        auto& block = (*blocks)[index];
        if (consumed < block.iov_len) {
          auto* data = static_cast<char*>(block.iov_base);
          block.iov_base = data + consumed;
          block.iov_len -= consumed;
          consumed = 0;
        } else {
          consumed -= block.iov_len;
          ++index;
        }
      }
    }
  }
  return true;
}

bool AppendSnapshotRecord(std::string_view key, const db::RedisObject& object,
                          std::optional<int64_t> expire,
                          std::deque<std::string>* const snapshot) {
  snapshot->emplace_back();
  auto& record = snapshot->back();
  bool encoded = false;
  switch (object.Type()) {
    case db::RedisObject::ObjectType::kString:
      encoded = AppendFixedCommand({"SET", key, object.String()}, &record);
      break;
    case db::RedisObject::ObjectType::kSet: {
      const auto* set = object.Set();
      const size_t size = set->Size();
      size_t index = 0;
      encoded = set->ForEachMember(
          [key, size, &index, &record](std::string_view member) {
            if (!BeginSnapshotBatch("SADD", key, index, size, 1, &record)) {
              return false;
            }
            reply::AppendBulkString(member, &record);
            ++index;
            return true;
          });
      break;
    }
    case db::RedisObject::ObjectType::kList: {
      const auto* list = object.List();
      const size_t size = list->Size();
      size_t index = 0;
      if (size == 0) {
        encoded = true;
        break;
      }
      encoded = list->ForEach(
          0, size - 1, [key, size, &index, &record](std::string_view value) {
            if (!BeginSnapshotBatch("RPUSH", key, index, size, 1, &record)) {
              return false;
            }
            reply::AppendBulkString(value, &record);
            ++index;
            return true;
          });
      break;
    }
    case db::RedisObject::ObjectType::kZSet: {
      const auto* zset = object.ZSet();
      const size_t size = zset->Size();
      size_t index = 0;
      encoded = zset->ForEachEntry(
          [key, size, &index, &record](std::string_view member, double score) {
            if (!BeginSnapshotBatch("ZADD", key, index, size, 2, &record)) {
              return false;
            }
            const std::string score_text = utils::FloatToString(score);
            reply::AppendBulkString(score_text, &record);
            reply::AppendBulkString(member, &record);
            ++index;
            return true;
          });
      break;
    }
    case db::RedisObject::ObjectType::kHash: {
      const auto* hash = object.Hash();
      const size_t size = hash->Size();
      size_t index = 0;
      encoded = hash->ForEachEntry(
          [key, size, &index, &record](std::string_view field,
                                       std::string_view value) {
            if (!BeginSnapshotBatch("HSET", key, index, size, 2, &record)) {
              return false;
            }
            reply::AppendBulkString(field, &record);
            reply::AppendBulkString(value, &record);
            ++index;
            return true;
          });
      break;
    }
  }
  if (!encoded ||
      (expire.has_value() && !AppendExpireAt(key, *expire, &record))) {
    snapshot->pop_back();
    return false;
  }
  if (record.empty()) {
    snapshot->pop_back();
  }
  return true;
}

bool BuildSnapshot(db::RedisDb* const db,
                   std::deque<std::string>* const snapshot) {
  return db != nullptr && snapshot != nullptr &&
         db->ForEachObject([db, snapshot](std::string_view key,
                                          const db::RedisObject& object) {
           return AppendSnapshotRecord(key, object, db->Expiration(key),
                                       snapshot);
         });
}
}  // namespace

std::unique_ptr<Aof> Aof::Open(const Options& options, db::RedisDb* const db) {
  if (db == nullptr || options.path.empty()) {
    return nullptr;
  }
  const int fd = open(options.path.c_str(), O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
    return nullptr;
  }
  auto aof = std::unique_ptr<Aof>(new Aof(fd, options.path, options.fsync));
  if (!aof->Replay(db) || lseek(fd, 0, SEEK_END) < 0) {
    return nullptr;
  }
  if (options.fsync != FsyncPolicy::kAlways) {
    aof->worker_ = std::thread([instance = aof.get()] { instance->Run(); });
  }
  return aof;
}

Aof::Aof(int fd, std::string path, FsyncPolicy fsync)
    : fd_(fd),
      path_(std::move(path)),
      fsync_(fsync),
      last_sync_(std::chrono::steady_clock::now()) {}

bool Aof::Append(std::string_view command,
                 const std::vector<std::string_view>& args,
                 db::RedisDb* const db) {
  std::string encoded;
  if (!Encode(command, args, db, &encoded)) {
    return false;
  }
  if (encoded.empty()) {
    return true;
  }

  if (fsync_ == FsyncPolicy::kAlways) {
    const std::scoped_lock lock(mutex_);
    if (!healthy_ || stopping_) {
      return false;
    }
    BufferRewriteCommandLocked(encoded);
    if (!WriteAll(fd_, encoded) || !SyncFile(fd_)) {
      FailLocked();
    }
    return healthy_;
  }

  {
    const std::scoped_lock lock(mutex_);
    if (!healthy_ || stopping_ || encoded.size() > kMaxPendingBytes ||
        pending_bytes_ > kMaxPendingBytes - encoded.size() ||
        pending_.size() >= kMaxPendingCommands) {
      FailLocked();
      return false;
    }
    BufferRewriteCommandLocked(encoded);
    pending_bytes_ += encoded.size();
    pending_.push_back(std::move(encoded));
  }
  work_available_.notify_one();
  return true;
}

RewriteResult Aof::StartRewrite(db::RedisDb* const db) {
  std::thread completed_rewrite;
  {
    const std::scoped_lock lock(mutex_);
    if (!healthy_ || stopping_ || db == nullptr) {
      return RewriteResult::kError;
    }
    if (rewriting_) {
      return RewriteResult::kInProgress;
    }
    if (rewrite_worker_.joinable()) {
      completed_rewrite = std::move(rewrite_worker_);
    }
    rewriting_ = true;
    rewrite_failed_ = false;
    rewrite_pending_.clear();
    rewrite_pending_bytes_ = 0;
  }
  if (completed_rewrite.joinable()) {
    completed_rewrite.join();
  }

  std::deque<std::string> snapshot;
  if (!BuildSnapshot(db, &snapshot)) {
    const std::scoped_lock lock(mutex_);
    FinishRewriteLocked();
    return RewriteResult::kError;
  }

  std::string path_template = path_ + ".rewrite.XXXXXX";
  std::vector<char> writable_path(path_template.begin(), path_template.end());
  writable_path.push_back('\0');
  const int rewrite_fd = mkstemp(writable_path.data());
  if (rewrite_fd < 0) {
    const std::scoped_lock lock(mutex_);
    FinishRewriteLocked();
    return RewriteResult::kError;
  }

  struct stat file_info {};
  const mode_t mode = fstat(fd_, &file_info) == 0
                          ? static_cast<mode_t>(file_info.st_mode & 0777)
                          : static_cast<mode_t>(0644);
  if (fchmod(rewrite_fd, mode) < 0) {
    close(rewrite_fd);
    unlink(writable_path.data());
    const std::scoped_lock lock(mutex_);
    FinishRewriteLocked();
    return RewriteResult::kError;
  }

  try {
    rewrite_worker_ = std::thread(
        [this, rewrite_fd, temp_path = std::string(writable_path.data()),
         snapshot = std::move(snapshot)]() mutable {
          RunRewrite(rewrite_fd, temp_path, std::move(snapshot));
        });
  } catch (const std::system_error&) {
    close(rewrite_fd);
    unlink(writable_path.data());
    const std::scoped_lock lock(mutex_);
    FinishRewriteLocked();
    return RewriteResult::kError;
  }
  return RewriteResult::kStarted;
}

bool Aof::Healthy() const {
  const std::scoped_lock lock(mutex_);
  return healthy_;
}

void Aof::WaitUntilIdle() {
  if (fsync_ == FsyncPolicy::kAlways) {
    return;
  }
  std::unique_lock<std::mutex> lock(mutex_);
  idle_.wait(lock, [this] {
    return (!writing_ && !syncing_ && pending_.empty()) || !healthy_;
  });
}

void Aof::WaitUntilRewriteIdle() {
  std::unique_lock<std::mutex> lock(mutex_);
  rewrite_state_changed_.wait(lock, [this] { return !rewriting_; });
}

Aof::~Aof() {
  {
    const std::scoped_lock lock(mutex_);
    stopping_ = true;
  }
  work_available_.notify_all();
  rewrite_state_changed_.notify_all();
  if (rewrite_worker_.joinable()) {
    rewrite_worker_.join();
  }
  if (worker_.joinable()) {
    worker_.join();
  }
  close(fd_);
}

bool Aof::Replay(db::RedisDb* const db) {
  if (lseek(fd_, 0, SEEK_SET) < 0) {
    return false;
  }

  db->SetLoading(true);
  const bool replayed = [this, db] {
    Client client(db);
    std::array<char, kReadChunkSize> chunk{};
    std::string input;
    input.reserve(kReadChunkSize);
    command::CommandArgs args;
    size_t complete_bytes = 0;
    bool reached_end = false;

    while (!reached_end) {
      ssize_t bytes_read = -1;
      while (true) {
        bytes_read = read(fd_, chunk.data(), chunk.size());
        if (bytes_read >= 0 || errno != EINTR) {
          break;
        }
      }
      if (bytes_read < 0) {
        return false;
      }
      reached_end = bytes_read == 0;
      if (bytes_read > 0) {
        input.append(chunk.data(), static_cast<size_t>(bytes_read));
      }

      size_t consumed = 0;
      while (consumed < input.size()) {
        const std::string_view remaining(input.data() + consumed,
                                         input.size() - consumed);
        if (remaining.front() != '*') {
          return false;
        }
        std::string_view name;
        const auto parsed = request_parser::Parse(remaining, &name, &args);
        if (parsed.status == request_parser::ParseStatus::kIncomplete) {
          break;
        }
        if (parsed.status != request_parser::ParseStatus::kComplete) {
          return false;
        }
        const auto* metadata = command::Find(name);
        if (metadata == nullptr ||
            metadata->access != command::CommandAccess::kWrite ||
            !metadata->arity.Accepts(args.size()) ||
            !client.ExecuteForReplay(metadata, &args)) {
          return false;
        }
        consumed += parsed.consumed;
        complete_bytes += parsed.consumed;
      }
      if (consumed > 0) {
        input.erase(0, consumed);
      }
      if (input.size() > kMaxReplayCommandBytes) {
        return false;
      }
    }

    if (!input.empty() &&
        ftruncate(fd_, static_cast<off_t>(complete_bytes)) < 0) {
      return false;
    }
    return input.empty() || SyncFile(fd_);
  }();
  db->SetLoading(false);
  return replayed;
}

bool Aof::Encode(std::string_view command,
                 const std::vector<std::string_view>& args,
                 db::RedisDb* const db, std::string* const output) {
  if (db == nullptr || output == nullptr) {
    return false;
  }
  if (command == "SET") {
    if (args.empty()) {
      return false;
    }
    const auto* object = db->LookupKey(args[0]);
    if (object == nullptr) {
      return AppendSingleKeyCommand("DEL", args[0], output);
    }
    if (object->Type() != db::RedisObject::ObjectType::kString) {
      return false;
    }
    const auto expire = db->Expiration(args[0]);
    if (!ReserveSetRecord(args[0], object->String(), expire.has_value(),
                          output)) {
      return false;
    }
    reply::AppendArrayHeader(3, output);
    reply::AppendBulkString("SET", output);
    reply::AppendBulkString(args[0], output);
    reply::AppendBulkString(object->String(), output);
    return !expire.has_value() || AppendExpireAt(args[0], *expire, output);
  }
  if (command == "EXPIRE" || command == "PEXPIRE" || command == "PEXPIREAT") {
    if (args.empty()) {
      return false;
    }
    const auto expire = db->Expiration(args[0]);
    if (expire.has_value()) {
      return AppendExpireAt(args[0], *expire, output);
    }
    return AppendSingleKeyCommand("DEL", args[0], output);
  }
  return AppendCommand(command, args, output);
}

void Aof::Run() {
  std::deque<std::string> batch;
  std::vector<iovec> blocks;
  while (true) {
    bool should_sync = false;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      while (healthy_ && pending_.empty() && !stopping_) {
        if (fsync_ == FsyncPolicy::kEverySecond && dirty_) {
          const auto deadline = last_sync_ + kSyncInterval;
          if (!work_available_.wait_until(lock, deadline, [this] {
                return stopping_ || !pending_.empty() || !healthy_;
              })) {
            break;
          }
        } else {
          work_available_.wait(lock, [this] {
            return stopping_ || !pending_.empty() || !healthy_;
          });
        }
      }
      if (!healthy_) {
        FailLocked();
        return;
      }
      if (!pending_.empty()) {
        batch.swap(pending_);
        pending_bytes_ = 0;
        writing_ = true;
      } else if (dirty_ &&
                 (stopping_ || (fsync_ == FsyncPolicy::kEverySecond &&
                                std::chrono::steady_clock::now() - last_sync_ >=
                                    kSyncInterval))) {
        syncing_ = true;
        should_sync = true;
      } else if (stopping_) {
        return;
      }
    }

    if (!batch.empty()) {
      const bool written = WriteBatch(fd_, batch, &blocks);
      {
        const std::scoped_lock lock(mutex_);
        writing_ = false;
        if (!written) {
          FailLocked();
          return;
        }
        dirty_ = true;
        NotifyIfIdleLocked();
      }
      batch.clear();
      continue;
    }

    if (should_sync) {
      const bool synced = Sync();
      {
        const std::scoped_lock lock(mutex_);
        syncing_ = false;
        if (!synced) {
          FailLocked();
          return;
        }
        dirty_ = false;
        last_sync_ = std::chrono::steady_clock::now();
        NotifyIfIdleLocked();
      }
    }
  }
}

void Aof::RunRewrite(int rewrite_fd, const std::string& temp_path,
                     std::deque<std::string> snapshot) {
  std::deque<std::string> batch;
  std::vector<iovec> blocks;
  bool succeeded = WriteBatch(rewrite_fd, snapshot, &blocks);
  snapshot.clear();

  while (succeeded) {
    std::unique_lock<std::mutex> lock(mutex_);
    rewrite_state_changed_.wait(lock, [this] {
      return !healthy_ || rewrite_failed_ || !rewrite_pending_.empty() ||
             (!writing_ && !syncing_ && pending_.empty());
    });
    if (!healthy_ || rewrite_failed_) {
      break;
    }
    if (!rewrite_pending_.empty()) {
      batch.swap(rewrite_pending_);
      rewrite_pending_bytes_ = 0;
      lock.unlock();
      succeeded = WriteBatch(rewrite_fd, batch, &blocks);
      batch.clear();
      continue;
    }

    succeeded = SyncFile(rewrite_fd) &&
                std::rename(temp_path.c_str(), path_.c_str()) == 0;
    if (succeeded) {
      const int old_fd = fd_;
      fd_ = rewrite_fd;
      rewrite_fd = -1;
      dirty_ = false;
      last_sync_ = std::chrono::steady_clock::now();
      close(old_fd);
    }
    FinishRewriteLocked();
    work_available_.notify_all();
    lock.unlock();
    if (rewrite_fd >= 0) {
      close(rewrite_fd);
      unlink(temp_path.c_str());
    }
    if (!succeeded) {
      RS_LOG_DEBUG("AOF rewrite failed\n");
    }
    return;
  }

  close(rewrite_fd);
  unlink(temp_path.c_str());
  {
    const std::scoped_lock lock(mutex_);
    FinishRewriteLocked();
  }
  RS_LOG_DEBUG("AOF rewrite failed\n");
}

void Aof::BufferRewriteCommandLocked(const std::string& command) {
  if (!rewriting_ || rewrite_failed_) {
    return;
  }
  if (command.size() > kMaxPendingBytes ||
      rewrite_pending_bytes_ > kMaxPendingBytes - command.size() ||
      rewrite_pending_.size() >= kMaxPendingCommands) {
    rewrite_failed_ = true;
    rewrite_pending_.clear();
    rewrite_pending_bytes_ = 0;
  } else {
    rewrite_pending_bytes_ += command.size();
    rewrite_pending_.push_back(command);
  }
  rewrite_state_changed_.notify_all();
}

void Aof::FinishRewriteLocked() {
  rewrite_pending_.clear();
  rewrite_pending_bytes_ = 0;
  rewriting_ = false;
  rewrite_failed_ = false;
  rewrite_state_changed_.notify_all();
}

bool Aof::Sync() const { return SyncFile(fd_); }

void Aof::FailLocked() {
  healthy_ = false;
  pending_.clear();
  pending_bytes_ = 0;
  writing_ = false;
  syncing_ = false;
  dirty_ = false;
  idle_.notify_all();
  rewrite_state_changed_.notify_all();
}

void Aof::NotifyIfIdleLocked() {
  rewrite_state_changed_.notify_all();
  if (!writing_ && !syncing_ && pending_.empty()) {
    idle_.notify_all();
  }
}
}  // namespace redis_simple::aof
