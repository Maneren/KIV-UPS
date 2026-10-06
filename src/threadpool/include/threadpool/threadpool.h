#pragma once

#include <chrono>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace threadpool {

template <typename R> class pool_future;

#if __cpp_lib_move_only_function >= 202110L
using task_type = std::move_only_function<void()>;
#else
using task_type = std::function<void()>;
#endif

/**
 * @class Threadpool
 * @brief A thread pool for managing and executing tasks concurrently.
 *
 * This class provides functionality to spawn tasks with or without return
 * values (futures) and supports parallel processing of ranges. It simplifies
 * concurrent task execution by managing a pool of worker threads.
 *
 * @note By default, the thread pool size is determined by the number of
 * hardware threads available on the system.
 */
class Threadpool {
public:
  /**
   * @brief Construct a new Threadpool object.
   * @param thread_count The number of worker threads to create. Defaults to the
   * number of CPU threads.
   */
  Threadpool(size_t thread_count = std::thread::hardware_concurrency());

  /**
   * @brief Destroy the Threadpool object and joins all worker threads.
   */
  ~Threadpool();

  /**
   * @brief Delete copy constructor.
   */
  Threadpool(const Threadpool &) = delete;

  /**
   * @brief Delete copy assignment operator.
   */
  Threadpool &operator=(const Threadpool &) = delete;

  /**
   * @brief Move construct a new Threadpool object.
   * @param other The other Threadpool to move from.
   */
  Threadpool(Threadpool &&other) noexcept;

  /**
   * @brief Move assign a Threadpool object.
   *
   * @param other The other Threadpool to move from.
   *
   * @return Threadpool& Reference to this Threadpool.
   */
  Threadpool &operator=(Threadpool &&other) noexcept;

  /**
   * @brief Spawn a task and returns a future to retrieve its result.
   *
   * @tparam Functor The type of the callable object to execute.
   * @tparam Args The types of the arguments to pass to the callable object.
   * @tparam Result The return type of the callable object.
   *
   * @param f The callable object to execute.
   * @param args The arguments to pass to the callable object.
   *
   * @return std::future<Result> A future that can be used to retrieve the
   * result of the task once it completes.
   */
  template <
      typename Functor,
      typename... Args,
      typename Result = std::invoke_result_t<Functor, Args...>>
  pool_future<Result> spawn_with_future(Functor &&f, Args &&...args)
    requires(std::is_invocable_v<Functor, Args...>)
  {
    // Helper to package and enqueue a bound callable.
    auto spawn_packaged = [this](auto &&bound) {
      std::packaged_task<Result()> task(std::forward<decltype(bound)>(bound));
      pool_future<Result> fut(task.get_future(), *this);
      spawn(task_type([task = std::move(task)]() mutable { task(); }));
      return fut;
    };

    if constexpr (std::is_lvalue_reference_v<Functor>) {
      // Referenced task: no copy/move of the functor. Caller must keep
      // `f` alive until the future is ready.
      auto bound = [&f,
                    ... args = std::forward<Args>(args)]() mutable -> Result {
        return f(std::forward<Args>(args)...);
      };
      return spawn_packaged(std::move(bound));
    } else {
      // Owned task: move the functor into the queue.
      auto bound = [f = std::forward<Functor>(f),
                    ... args = std::forward<Args>(args)]() mutable -> Result {
        return f(std::forward<Args>(args)...);
      };
      return spawn_packaged(std::move(bound));
    }
  }

  /**
   * @brief Spawn a task to be executed by the thread pool.
   *
   * @param task A callable object representing the task to execute.
   */
  void spawn(task_type &&task);

  /**
   * @brief Spawn a background task inside the thread pool.
   *
   * @tparam F The type of the function to execute.
   * @tparam Args The types of the arguments to pass to the function.
   * @tparam U The return type of the function.
   *
   * @param f The function to execute.
   * @param args The arguments to pass to the function.
   */
  template <typename Functor, typename... Args>
  void spawn(Functor &&f, Args &&...args)
    requires(std::is_same_v<std::invoke_result_t<Functor, Args...>, void>)
  {
    spawn(task_type{std::forward<Functor>(f), std::forward<Args>(args)...});
  }

  /**
   * @brief Gracefully stop the thread pool and joins all worker threads.
   *
   * This method ensures that all tasks are completed before the thread pool
   * shuts down. It is automatically called by the destructor.
   */
  void join();

private:
  std::vector<std::jthread> mWorkers;
  std::queue<task_type> mTasks;
  std::mutex mMutex;
  std::condition_variable mCondition;
  bool mRunning = true;

  template <typename R> friend class pool_future;
};

/** @brief A helper class to retrieve the result of a future from a thread pool.
 *
 * This class wraps a std::future and ensures that calling spawn recursively
 * will not cause a deadlock. It is used internally by the Threadpool class.
 *
 * The `get` method is a blocking call that retrieves the result of the future
 * while executing tasks from the thread pool in the meantime.
 *
 * @tparam R The type of the result to retrieve.
 */
template <typename R> class pool_future : public std::future<R> {
public:
  pool_future(std::future<R> &&future, Threadpool &pool)
      : std::future<R>(std::move(future)), mPool(pool) {}

  R get() {
    // Wait until our shared state is ready, executing queued pool tasks in
    // the meantime. This lets a task block on a child task it spawned
    // without deadlocking, even with a single worker thread.
    while (this->wait_for(std::chrono::seconds(0)) !=
           std::future_status::ready) {
      task_type task;
      {
        std::unique_lock<std::mutex> lock(mPool.mMutex);
        if (mPool.mTasks.empty()) {
          lock.unlock();
          std::this_thread::yield();
          continue;
        }

        task = std::move(mPool.mTasks.front());
        mPool.mTasks.pop();
      }
      task();
    }
    return std::future<R>::get();
  }

private:
  Threadpool &mPool;
};

/**
 * @brief A global thread pool instance initialized at program startup.
 */
extern Threadpool global;

} // namespace threadpool
