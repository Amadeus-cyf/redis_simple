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

enum class RewriteResult {
  kStarted,
  kInProgress,
  kError,
};

class Aof {
 public:
  static std::unique_ptr<Aof> Open(const Options& options, db::RedisDb* db);
  Aof(const Aof&) = delete;
  Aof& operator=(const Aof&) = delete;
  bool Append(std::string_view command,
              const std::vector<std::string_view>& args, db::RedisDb* db);
  RewriteResult StartRewrite(db::RedisDb* db);
  bool Healthy() const;
  void WaitUntilIdle();
  void WaitUntilRewriteIdle();
  ~Aof();

 private:
  Aof(int fd, std::string path, FsyncPolicy fsync);
  bool Replay(db::RedisDb* db);
  static bool Encode(std::string_view command,
                     const std::vector<std::string_view>& args, db::RedisDb* db,
                     std::string* output);
  void Run();
  void RunRewrite(int rewrite_fd, const std::string& temp_path,
                  std::deque<std::string> snapshot);
  void BufferRewriteCommandLocked(const std::string& command);
  void FinishRewriteLocked();
  bool Sync() const;
  void FailLocked();
  void NotifyIfIdleLocked();

  static constexpr size_t kMaxPendingBytes = size_t{64} * 1024 * 1024;
  static constexpr size_t kMaxPendingCommands = size_t{64} * 1024;
  int fd_;
  std::string path_;
  FsyncPolicy fsync_;
  mutable std::mutex mutex_;
  std::condition_variable work_available_;
  std::condition_variable idle_;
  std::condition_variable rewrite_state_changed_;
  std::deque<std::string> pending_;
  std::deque<std::string> rewrite_pending_;
  std::thread worker_;
  std::thread rewrite_worker_;
  size_t pending_bytes_{};
  size_t rewrite_pending_bytes_{};
  bool writing_{};
  bool syncing_{};
  bool dirty_{};
  bool stopping_{};
  bool healthy_{true};
  bool rewriting_{};
  bool rewrite_failed_{};
  std::chrono::steady_clock::time_point last_sync_;
};
}  // namespace redis_simple::aof
