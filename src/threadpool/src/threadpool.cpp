#include <threadpool/threadpool.h>
#include <utility>

namespace threadpool {

Threadpool::Threadpool(size_t thread_count) {
  if (thread_count == 0) {
    thread_count = std::thread::hardware_concurrency();
  }

  mWorkers.reserve(thread_count);

  for (std::size_t i = 0; i < thread_count; ++i) {
    mWorkers.emplace_back([this] {
      while (true) {
        task_type task;
        {
          std::unique_lock<std::mutex> lock(mMutex);
          mCondition.wait(
              lock,
              // shutting down or task available
              [this] { return !mRunning || !mTasks.empty(); }
          );

          if (!mRunning && mTasks.empty()) {
            return;
          }

          task = std::move(mTasks.front());
          mTasks.pop();
        }
        task();
      }
    });
  }
}

Threadpool::~Threadpool() { join(); }

Threadpool::Threadpool(Threadpool &&other) noexcept
    : mWorkers(std::move(other.mWorkers)), mTasks(std::move(other.mTasks)),
      mRunning(other.mRunning) {}

Threadpool &Threadpool::operator=(Threadpool &&other) noexcept {
  mWorkers = std::move(other.mWorkers);
  mTasks = std::move(other.mTasks);
  mRunning = other.mRunning;
  return *this;
}

void Threadpool::spawn(task_type &&task) {
  {
    std::unique_lock<std::mutex> lock(mMutex);
    mTasks.emplace(std::move(task));
  }
  mCondition.notify_one();
}

void Threadpool::join() {
  {
    std::unique_lock<std::mutex> lock(mMutex);
    mRunning = false;
  }
  mCondition.notify_all();
  mWorkers.clear(); // joins the threads
}

Threadpool global{};

} // namespace threadpool
