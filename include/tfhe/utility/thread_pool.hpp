// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_UTILITY_THREAD_POOL_HPP
#define TFHE_UTILITY_THREAD_POOL_HPP

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

// Fixed-size pool of worker threads, all spun up in the constructor and
// parked on a shared task queue until work (or destruction) wakes them.
// submit() hands back a std::future so callers can wait on -- or collect
// the result of -- whichever job they just queued.
//
// num_threads == 0 disables threading entirely: no std::thread is ever
// created, and submit() just runs the task inline on the caller and
// returns an already-ready future. That makes ThreadPool safe to
// instantiate even where std::thread isn't usable (e.g. an Emscripten
// build without -pthread), without callers needing their own fallback.
class ThreadPool {
 public:
  explicit ThreadPool(uint32_t num_threads) {
    workers_.reserve(num_threads);
    for (uint32_t i = 0; i < num_threads; ++i) {
      workers_.emplace_back([this] { worker_loop(); });
    }
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  ~ThreadPool() {
    if (!workers_.empty()) {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
      }
      cv_.notify_all();
      for (std::thread& w : workers_) w.join();
    }
  }

  // Queues f for execution and returns a future for its result. With no
  // worker threads (see above), f runs synchronously before this returns.
  template <typename F>
  auto submit(F&& f) -> std::future<std::invoke_result_t<F>> {
    using R = std::invoke_result_t<F>;

    if (workers_.empty()) {
      std::promise<R> promise;
      std::future<R> fut = promise.get_future();
      run_and_fulfill(promise, f);
      return fut;
    }

    auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
    std::future<R> fut = task->get_future();
    {
      std::lock_guard<std::mutex> lock(mutex_);
      tasks_.emplace([task] { (*task)(); });
    }
    cv_.notify_one();
    return fut;
  }

  uint32_t size() const noexcept {
    return static_cast<uint32_t>(workers_.size());
  }

 private:
  template <typename R, typename F>
  static void run_and_fulfill(std::promise<R>& promise, F& f) {
    if constexpr (std::is_void_v<R>) {
      f();
      promise.set_value();
    } else {
      promise.set_value(f());
    }
  }

  void worker_loop() {
    for (;;) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
        if (stop_ && tasks_.empty()) return;
        task = std::move(tasks_.front());
        tasks_.pop();
      }
      task();
    }
  }

  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool stop_ = false;
};

#endif  // TFHE_UTILITY_THREAD_POOL_HPP
