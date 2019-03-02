#pragma once

#include <functional>
#include <mutex>
#include <queue>

namespace fas
{
  /**
   * @brief Executor which pushes jobs to a queue
   *
   * The executor provides a run function which executes all of the queued jobs, until the queue is empty.
   */
  class QueueExecutor
  {
  public:
    QueueExecutor() = default;
    QueueExecutor(QueueExecutor&) = delete;
    QueueExecutor& operator=(QueueExecutor&) = delete;

    void spawn(std::function<void()> fn)
    {
      const std::lock_guard<std::mutex> lock(_queueMutex);
      _jobs.push(fn);
    }

    void run()
    {
      while (true)
      {
        std::function<void()> job;
        {
          std::lock_guard<std::mutex> lock(_queueMutex);
          if (_jobs.empty())
            return;
          job = std::move(_jobs.front());
          _jobs.pop();
        }
        job();
      }
    }

    std::size_t jobCount() const
    {
      const std::lock_guard<std::mutex> lock(_queueMutex);
      const auto size = _jobs.size();
      return size;
    }

  private:
    mutable std::mutex _queueMutex;
    std::queue<std::function<void()>> _jobs;
  };
} // namespace fass
