#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace redis_simple::db {
class RedisDb;
}

namespace redis_simple::aof {
enum class FsyncPolicy {
  kAlways,
  kEverySecond,
  kNo,
};

struct Options {
  std::string path{"appendonly.aof"};
  FsyncPolicy fsync{FsyncPolicy::kEverySecond};
};

class Aof {
 public:
  static std::unique_ptr<Aof> Open(const Options& options, db::RedisDb* db);
  Aof(const Aof&) = delete;
  Aof& operator=(const Aof&) = delete;
  bool Append(std::string_view command,
              const std::vector<std::string_view>& args, db::RedisDb* db);
  bool Healthy() const;
  void WaitUntilIdle();
  ~Aof();

 private:
  Aof(int fd, FsyncPolicy fsync);
  bool Replay(db::RedisDb* db);
  static bool Encode(std::string_view command,
                     const std::vector<std::string_view>& args, db::RedisDb* db,
                     std::string* output);
  void Run();
  bool Sync() const;
  void FailLocked();
  void NotifyIfIdleLocked();

  static constexpr size_t kMaxPendingBytes = size_t{64} * 1024 * 1024;
  static constexpr size_t kMaxPendingCommands = size_t{64} * 1024;
  int fd_;
  FsyncPolicy fsync_;
  mutable std::mutex mutex_;
  std::condition_variable work_available_;
  std::condition_variable idle_;
  std::deque<std::string> pending_;
  std::thread worker_;
  size_t pending_bytes_{};
  bool writing_{};
  bool syncing_{};
  bool dirty_{};
  bool stopping_{};
  bool healthy_{true};
  std::chrono::steady_clock::time_point last_sync_;
};
}  // namespace redis_simple::aof
