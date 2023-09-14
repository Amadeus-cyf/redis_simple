#pragma once

#include <future>

namespace redis_simple {
namespace cli {
template <typename T>
class CompletableFuture {
 public:
  using callback = T (*)(const T&);
  explicit CompletableFuture(std::future<T>&& future)
      : future(std::move(future)){};
  CompletableFuture(const CompletableFuture&) = delete;
  CompletableFuture& operator=(const CompletableFuture&) = delete;
  CompletableFuture thenApply(callback cb);
  CompletableFuture thenApplyAsync(callback cb);
  T get();

 private:
  std::future<T> future;
};

template <typename T>
CompletableFuture<T> CompletableFuture<T>::thenApply(callback cb) {
  future.wait();
  std::promise<T> promise;
  promise.set_value(cb(future.get()));
  return CompletableFuture(promise.get_future());
}

template <typename T>
CompletableFuture<T> CompletableFuture<T>::thenApplyAsync(callback cb) {
  return CompletableFuture(std::async(std::launch::async, [=]() {
    future.wait();
    return cb(future.get());
  }));
}

template <typename T>
T CompletableFuture<T>::get() {
  return future.get();
}
}  // namespace cli
}  // namespace redis_simple
