#pragma once

#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace fas
{
  /**
   * @brief Sequential executor running in its own thread
   */
  class ThreadedExecutor
  {
  public:
    ThreadedExecutor() = default;
    ThreadedExecutor(ThreadedExecutor&) = delete;
    ThreadedExecutor& operator=(ThreadedExecutor&) = delete;

    ~ThreadedExecutor()
    {
      if (_thread.joinable())
        stop();
    }

    void spawn(std::function<void()> fn)
    {
      bool wakeUpThread = false;
      {
        const std::lock_guard<std::mutex> lock(_jobsMutex);
        wakeUpThread = _jobs.empty();
        _jobs.push(fn);
      }
      if (wakeUpThread)
        _jobsAvailable.notify_one();
    }

    void start()
    {
      assert(!_thread.joinable());
      _quitFlag = false;
      _thread = std::thread([this]() { this->threadMain(); });
    }

    void threadMain()
    {
      while (true)
      {
        std::function<void()> job;
        {
          std::unique_lock<std::mutex> lock(_jobsMutex);
          while (!_quitFlag && _jobs.empty())
            _jobsAvailable.wait(lock);
          if (_quitFlag)
            return;
          job = std::move(_jobs.front());
          _jobs.pop();
        }
        job();
      }
    }

    void stop()
    {
      assert(_thread.joinable());
      {
        std::lock_guard<std::mutex> lock(_jobsMutex);
        _quitFlag = true;
      }
      _jobsAvailable.notify_one();
      if (_thread.joinable())
        _thread.join();
    }

  private:
    std::thread _thread;
    std::mutex _jobsMutex;
    std::condition_variable _jobsAvailable;
    bool _quitFlag;
    std::queue<std::function<void()>> _jobs;
  };
} // namespace fass
