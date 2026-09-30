
#include <chrono>
#include <gtest/gtest.h>
#include <ranges>
#include <thread>
#include <threadpool/threadpool.h>
#include <utils/print.h>

class ThreadPoolTest : public ::testing::Test {};

struct CopyAndMoveTester {
  static size_t constructor_count;
  static size_t copy_count;
  static size_t move_count;

  size_t n;

  CopyAndMoveTester(size_t n) : n(n) {
    constructor_count++;
    std::println("Constructor with n = {}", n);
  }

  CopyAndMoveTester(const CopyAndMoveTester &other) : n(other.n) {
    std::println("Copy constructor");
    copy_count++;
  }

  CopyAndMoveTester(CopyAndMoveTester &&other) noexcept : n(other.n) {
    std::println("Move constructor");
    move_count++;
  }

  CopyAndMoveTester &operator=(const CopyAndMoveTester &other) {
    std::println("Copy assignment");
    copy_count++;
    n = other.n;
    return *this;
  }

  CopyAndMoveTester &operator=(CopyAndMoveTester &&other) noexcept {
    std::println("Move assignment");
    move_count++;
    n = other.n;
    return *this;
  }

  size_t operator()() const { return n; }

  static void reset_counters() {
    constructor_count = 0;
    copy_count = 0;
    move_count = 0;
  }

  static auto get_counts() {
    return std::make_tuple(constructor_count, copy_count, move_count);
  }
};

size_t CopyAndMoveTester::constructor_count = 0;
size_t CopyAndMoveTester::copy_count = 0;
size_t CopyAndMoveTester::move_count = 0;

TEST_F(ThreadPoolTest, ThreadPoolTest) {
  threadpool::Threadpool pool;

  CopyAndMoveTester::reset_counters();
  constexpr size_t N = 10;
  std::vector<std::future<size_t>> futures;
  futures.reserve(N);
  for (size_t i : std::views::iota(size_t{0}, N)) {
    futures.push_back(pool.spawn_with_future(CopyAndMoveTester(i)));
  }

  for (auto [i, fut] : std::views::enumerate(futures)) {
    EXPECT_EQ(fut.get(), i);
  }

  auto [constructor_count, copy_count, move_count] =
      CopyAndMoveTester::get_counts();

  EXPECT_EQ(constructor_count, N);
  EXPECT_EQ(copy_count, 0);
  EXPECT_EQ(move_count, 2 * N);

  std::vector<CopyAndMoveTester> tasks;
  tasks.reserve(N);
  for (size_t i : std::views::iota(size_t{0}, N)) {
    tasks.emplace_back(i);
  }
  CopyAndMoveTester::reset_counters();

  std::vector<std::future<size_t>> futures2;
  futures2.reserve(N);
  for (CopyAndMoveTester &task : tasks) {
    futures2.push_back(pool.spawn_with_future(task));
  }

  for (auto [i, fut] : std::views::enumerate(futures2)) {
    EXPECT_EQ(fut.get(), i);
  }

  auto [constructor_count2, copy_count2, move_count2] =
      CopyAndMoveTester::get_counts();

  // std::destroy(pool);

  EXPECT_EQ(constructor_count2, 0);
  EXPECT_EQ(copy_count2, 0);
  EXPECT_EQ(move_count2, 0);
}

TEST_F(ThreadPoolTest, RecursiveSpawnDoesNotDeadlock) {
  // A single worker guarantees a deadlock with a blocking wait: the worker
  // runs the outer task while the inner task sits in the queue. pool_future
  // avoids it by executing queued tasks while waiting.
  threadpool::Threadpool pool(1);

  constexpr size_t N = 10;
  std::vector<threadpool::pool_future<size_t>> outers;
  outers.reserve(N);
  for (size_t i : std::views::iota(size_t{0}, N)) {
    outers.push_back(pool.spawn_with_future([&pool, i] {
      auto inner = pool.spawn_with_future([i] { return i * 2; });
      return inner.get();
    }));
  }

  for (auto [i, outer] : std::views::enumerate(outers)) {
    ASSERT_EQ(
      outer.wait_for(std::chrono::seconds(10)), std::future_status::ready
    ) << "deadlock: outer task " << i << " not ready within 10s";
    EXPECT_EQ(outer.get(), i * 2);
  }
}
