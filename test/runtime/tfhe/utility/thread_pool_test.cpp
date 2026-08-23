#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <vector>

#include "tfhe/utility/thread_pool.hpp"

TEST(ThreadPoolTest, RunsJobsAndReturnsPerJobResults) {
  ThreadPool pool(4);

  constexpr uint32_t kJobs = 64;
  std::vector<std::future<uint32_t>> futures;
  futures.reserve(kJobs);
  for (uint32_t i = 0; i < kJobs; ++i) {
    futures.push_back(pool.submit([i] { return i * i; }));
  }
  for (uint32_t i = 0; i < kJobs; ++i) {
    EXPECT_EQ(futures[i].get(), i * i);
  }
}

TEST(ThreadPoolTest, DistinctSlotsWrittenConcurrentlyStayIndependent) {
  ThreadPool pool(4);

  constexpr uint32_t kSlots = 64;
  std::vector<uint32_t> res(kSlots, 0);
  std::vector<std::future<void>> futures;
  futures.reserve(kSlots);
  for (uint32_t i = 0; i < kSlots; ++i) {
    futures.push_back(pool.submit([i, &res] { res[i] = i + 1; }));
  }
  for (std::future<void>& f : futures) f.get();

  for (uint32_t i = 0; i < kSlots; ++i) {
    EXPECT_EQ(res[i], i + 1);
  }
}

TEST(ThreadPoolTest, ZeroThreadsRunsJobsInlineSynchronously) {
  ThreadPool pool(0);
  EXPECT_EQ(pool.size(), 0u);

  std::future<uint32_t> fut = pool.submit([] { return 42u; });
  // With no worker threads, submit() must have already run the job by the
  // time it returns -- the future is ready without needing a wait.
  EXPECT_EQ(fut.wait_for(std::chrono::seconds(0)), std::future_status::ready);
  EXPECT_EQ(fut.get(), 42u);
}

TEST(ThreadPoolTest, SizeReflectsThreadCount) {
  ThreadPool pool(3);
  EXPECT_EQ(pool.size(), 3u);
}
