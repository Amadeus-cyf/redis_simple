#pragma once

#include <future>

namespace redis_simple {
namespace cli {
template <typename T>
class CompletableFuture {
 public:
  using callback = T (*)(const T&);
  explicit CompletableFuture(std::future<T>&& future)
      : future_(std::move(future)){};
  CompletableFuture(const CompletableFuture&) = delete;
  CompletableFuture& operator=(const CompletableFuture&) = delete;
  CompletableFuture ThenApply(callback cb);
  CompletableFuture ThenApplyAsync(callback cb);
  T Get();

 private:
  std::future<T> future_;
};

template <typename T>
CompletableFuture<T> CompletableFuture<T>::ThenApply(callback cb) {
  future_.wait();
  std::promise<T> promise;
  promise.set_value(cb(future_.get()));
  return CompletableFuture(promise.get_future());
}

template <typename T>
CompletableFuture<T> CompletableFuture<T>::ThenApplyAsync(callback cb) {
  return CompletableFuture(std::async(std::launch::async, [=]() {
    future_.wait();
    return cb(future_.get());
  }));
}

template <typename T>
T CompletableFuture<T>::Get() {
  return future_.get();
}
}  // namespace cli
}  // namespace redis_simple
