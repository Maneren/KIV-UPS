
#include <chrono>
#include <functional>
#include <future>
#include <gtest/gtest.h>
#include <print>
#include <ranges>
#include <threadpool/threadpool.h>
#include <utility>

namespace {

class ThreadPoolTest : public ::testing::Test {};

struct CopyAndMoveTester {
  static size_t constructor_count;
  static size_t copy_count;
  static size_t move_count;

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

  ~CopyAndMoveTester() = default;

  size_t operator()() const { return n; }

  static void reset_counters() {
    constructor_count = 0;
    copy_count = 0;
    move_count = 0;
  }

  static auto get_counts() {
    return std::make_tuple(constructor_count, copy_count, move_count);
  }

private:
  size_t n;
};

/// Counts copies/moves of itself; usable as an argument to verify forwarding.
struct CountingArg {
  static inline size_t copy_count = 0;
  static inline size_t move_count = 0;
  static inline void reset() {
    copy_count = 0;
    move_count = 0;
  }

  int value = 0;

  explicit CountingArg(int v = 0) : value(v) {}
  CountingArg(const CountingArg &other) : value(other.value) { ++copy_count; }
  CountingArg(CountingArg &&other) noexcept : value(other.value) {
    ++move_count;
  }
  CountingArg &operator=(const CountingArg &other) {
    value = other.value;
    ++copy_count;
    return *this;
  }
  CountingArg &operator=(CountingArg &&other) noexcept {
    value = other.value;
    ++move_count;
    return *this;
  }
  ~CountingArg() = default;
};

/// Counting functor returning a value; for spawn_with_future tests.
struct CountingTask {
  static inline size_t copy_count = 0;
  static inline size_t move_count = 0;
  static inline void reset() {
    copy_count = 0;
    move_count = 0;
  }

  int value = 0;

  explicit CountingTask(int v = 0) : value(v) {}
  CountingTask(const CountingTask &other) : value(other.value) { ++copy_count; }
  CountingTask(CountingTask &&other) noexcept : value(other.value) {
    ++move_count;
  }
  CountingTask &operator=(const CountingTask &other) {
    value = other.value;
    ++copy_count;
    return *this;
  }
  CountingTask &operator=(CountingTask &&other) noexcept {
    value = other.value;
    ++move_count;
    return *this;
  }
  ~CountingTask() = default;

  int operator()() const { return value; }
  int operator()(const CountingArg &arg) const { return value + arg.value; }
};

/// Counting functor returning void; for fire-and-forget spawn tests.
/// Reports its result through a promise (copies share the pointer).
struct VoidTask {
  static inline size_t copy_count = 0;
  static inline size_t move_count = 0;
  static inline void reset() {
    copy_count = 0;
    move_count = 0;
  }

  std::promise<int> *out = nullptr;
  int value = 0;

  VoidTask(int v, std::promise<int> &p) : out(&p), value(v) {}
  VoidTask(const VoidTask &other) : out(other.out), value(other.value) {
    ++copy_count;
  }
  VoidTask(VoidTask &&other) noexcept : out(other.out), value(other.value) {
    ++move_count;
  }
  VoidTask &operator=(const VoidTask &other) {
    out = other.out;
    value = other.value;
    ++copy_count;
    return *this;
  }
  VoidTask &operator=(VoidTask &&other) noexcept {
    out = other.out;
    value = other.value;
    ++move_count;
    return *this;
  }
  ~VoidTask() = default;

  void operator()() const { out->set_value(value); }
  void operator()(const CountingArg &arg) const {
    out->set_value(value + arg.value);
  }
};

/// Waits for a promise-based result with a deadlock-proof timeout.
void expect_result(std::future<int> &fut, int expected) {
  constexpr auto timeout = std::chrono::seconds(10);
  ASSERT_EQ(fut.wait_for(timeout), std::future_status::ready)
      << "task did not run within 10s";
  EXPECT_EQ(fut.get(), expected);
}

} // namespace

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
    ) << "deadlock: outer task "
      << i << " not ready within 10s";
    EXPECT_EQ(outer.get(), i * 2);
  }
}

TEST_F(ThreadPoolTest, SpawnValueCategories) {
  threadpool::Threadpool pool(4);

  // ---- spawn_with_future: functor value category (no args) ----
  {
    // lvalue: referenced, never copied or moved (caller keeps it alive
    // until get()).
    CountingTask::reset();
    CountingTask task(7);
    auto fut = pool.spawn_with_future(task);
    EXPECT_EQ(fut.get(), 7);
    EXPECT_EQ(CountingTask::copy_count, 0);
    EXPECT_EQ(CountingTask::move_count, 0);
  }
  {
    // prvalue: owned, moved but never copied.
    CountingTask::reset();
    auto fut = pool.spawn_with_future(CountingTask(8));
    EXPECT_EQ(fut.get(), 8);
    EXPECT_EQ(CountingTask::copy_count, 0);
    EXPECT_GT(CountingTask::move_count, 0);
  }
  {
    // xvalue: same ownership semantics as prvalue.
    CountingTask::reset();
    CountingTask task(9);
    auto fut = pool.spawn_with_future(std::move(task));
    EXPECT_EQ(fut.get(), 9);
    EXPECT_EQ(CountingTask::copy_count, 0);
    EXPECT_GT(CountingTask::move_count, 0);
  }

  // ---- spawn_with_future: argument value category ----
  {
    // lvalue arg: copied into the task exactly once.
    CountingTask::reset();
    CountingArg::reset();
    CountingArg arg(5);
    auto fut = pool.spawn_with_future(CountingTask(100), arg);
    EXPECT_EQ(fut.get(), 105);
    EXPECT_EQ(CountingTask::copy_count, 0);
    EXPECT_EQ(CountingArg::copy_count, 1);
  }
  {
    // prvalue arg: moved, never copied.
    CountingTask::reset();
    CountingArg::reset();
    auto fut = pool.spawn_with_future(CountingTask(100), CountingArg(5));
    EXPECT_EQ(fut.get(), 105);
    EXPECT_EQ(CountingArg::copy_count, 0);
    EXPECT_GT(CountingArg::move_count, 0);
  }
  {
    // xvalue arg: moved, never copied.
    CountingTask::reset();
    CountingArg::reset();
    CountingArg arg(5);
    auto fut = pool.spawn_with_future(CountingTask(100), std::move(arg));
    EXPECT_EQ(fut.get(), 105);
    EXPECT_EQ(CountingArg::copy_count, 0);
    EXPECT_GT(CountingArg::move_count, 0);
  }

  // ---- spawn (void): functor value category (no args) ----
  {
    // lvalue: copied into the task exactly once (fire-and-forget offers
    // no lifetime anchor for a reference).
    VoidTask::reset();
    std::promise<int> done;
    auto fut = done.get_future();
    VoidTask task(7, done);
    pool.spawn(task);
    expect_result(fut, 7);
    EXPECT_EQ(VoidTask::copy_count, 1);
  }
  {
    // prvalue: moved, never copied.
    VoidTask::reset();
    std::promise<int> done;
    auto fut = done.get_future();
    pool.spawn(VoidTask(8, done));
    expect_result(fut, 8);
    EXPECT_EQ(VoidTask::copy_count, 0);
    EXPECT_GT(VoidTask::move_count, 0);
  }
  {
    // xvalue: moved, never copied.
    VoidTask::reset();
    std::promise<int> done;
    auto fut = done.get_future();
    VoidTask task(9, done);
    pool.spawn(std::move(task));
    expect_result(fut, 9);
    EXPECT_EQ(VoidTask::copy_count, 0);
    EXPECT_GT(VoidTask::move_count, 0);
  }

  // ---- spawn (void): argument value category ----
  {
    // lvalue arg: copied into the task exactly once.
    VoidTask::reset();
    CountingArg::reset();
    std::promise<int> done;
    auto fut = done.get_future();
    VoidTask task(100, done);
    CountingArg arg(5);
    pool.spawn(task, arg);
    expect_result(fut, 105);
    EXPECT_EQ(VoidTask::copy_count, 1);
    EXPECT_EQ(CountingArg::copy_count, 1);
  }
  {
    // prvalue arg: moved, never copied.
    VoidTask::reset();
    CountingArg::reset();
    std::promise<int> done;
    auto fut = done.get_future();
    VoidTask task(100, done);
    pool.spawn(task, CountingArg(5));
    expect_result(fut, 105);
    EXPECT_EQ(CountingArg::copy_count, 0);
    EXPECT_GT(CountingArg::move_count, 0);
  }
  {
    // xvalue arg: moved, never copied.
    VoidTask::reset();
    CountingArg::reset();
    std::promise<int> done;
    auto fut = done.get_future();
    VoidTask task(100, done);
    CountingArg arg(5);
    pool.spawn(task, std::move(arg));
    expect_result(fut, 105);
    EXPECT_EQ(CountingArg::copy_count, 0);
    EXPECT_GT(CountingArg::move_count, 0);
  }

  // ---- std::ref: explicit opt-in to reference semantics ----
  {
    // spawn_with_future: referenced task itself is never copied or moved.
    CountingTask::reset();
    CountingTask task(11);
    auto fut = pool.spawn_with_future(std::ref(task));
    EXPECT_EQ(fut.get(), 11);
    EXPECT_EQ(CountingTask::copy_count, 0);
    EXPECT_EQ(CountingTask::move_count, 0);
  }
  {
    // spawn_with_future: referenced arg passes through without copying.
    CountingTask::reset();
    CountingArg::reset();
    CountingArg arg(5);
    auto fut = pool.spawn_with_future(CountingTask(100), std::ref(arg));
    EXPECT_EQ(fut.get(), 105);
    EXPECT_EQ(CountingArg::copy_count, 0);
    EXPECT_EQ(CountingArg::move_count, 0);
  }
  {
    // spawn (void): referenced task runs without being copied.
    VoidTask::reset();
    std::promise<int> done;
    auto fut = done.get_future();
    VoidTask task(12, done);
    pool.spawn(std::ref(task));
    expect_result(fut, 12);
    EXPECT_EQ(VoidTask::copy_count, 0);
    EXPECT_EQ(VoidTask::move_count, 0);
  }
  {
    // spawn (void): referenced functor and arg, no copies at all.
    VoidTask::reset();
    CountingArg::reset();
    std::promise<int> done;
    auto fut = done.get_future();
    VoidTask task(100, done);
    CountingArg arg(5);
    pool.spawn(std::ref(task), std::ref(arg));
    expect_result(fut, 105);
    EXPECT_EQ(VoidTask::copy_count, 0);
    EXPECT_EQ(VoidTask::move_count, 0);
    EXPECT_EQ(CountingArg::copy_count, 0);
    EXPECT_EQ(CountingArg::move_count, 0);
  }
}
