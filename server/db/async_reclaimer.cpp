#include "server/db/async_reclaimer.h"

#include <deque>
#include <mutex>
#include <utility>

namespace redis_simple::db {
AsyncReclaimer::AsyncReclaimer() : worker_([this] { Run(); }) {}

bool AsyncReclaimer::Reclaim(RedisObjectPtr object) {
  if (object == nullptr) {
    return false;
  }
  {
    const std::scoped_lock lock(mutex_);
    if (stopping_) {
      return false;
    }
    pending_.push_back(std::move(object));
  }
  work_available_.notify_one();
  return true;
}

void AsyncReclaimer::WaitUntilIdle() {
  std::unique_lock<std::mutex> lock(mutex_);
  idle_.wait(lock,
             [this] { return pending_.empty() && reclaiming_ == 0; });
}

size_t AsyncReclaimer::PendingCount() const {
  const std::scoped_lock lock(mutex_);
  return pending_.size() + reclaiming_;
}

AsyncReclaimer::~AsyncReclaimer() {
  {
    const std::scoped_lock lock(mutex_);
    stopping_ = true;
  }
  work_available_.notify_one();
  if (worker_.joinable()) {
    worker_.join();
  }
}

void AsyncReclaimer::Run() {
  while (true) {
    std::deque<RedisObjectPtr> batch;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      work_available_.wait(
          lock, [this] { return stopping_ || !pending_.empty(); });
      if (pending_.empty()) {
        if (stopping_) {
          return;
        }
        continue;
      }
      batch.swap(pending_);
      reclaiming_ += batch.size();
    }

    const size_t batch_size = batch.size();
    batch.clear();

    {
      const std::scoped_lock lock(mutex_);
      reclaiming_ -= batch_size;
      if (pending_.empty() && reclaiming_ == 0) {
        idle_.notify_all();
      }
    }
  }
}
}  // namespace redis_simple::db
