#include <threadpool/threadpool.h>

namespace threadpool {

Threadpool::Threadpool(size_t thread_count) {
  if (thread_count == 0) {
    thread_count = std::thread::hardware_concurrency();
  }

  mWorkers.reserve(thread_count);

  for (std::size_t i = 0; i < thread_count; ++i) {
    mWorkers.emplace_back([this] {
      while (true) {
        std::function<void()> task;
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
