#ifndef PERSISTENCE_OP_TASK_H
#define PERSISTENCE_OP_TASK_H

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace persistence
{
  namespace op
  {
    template <typename ResultT>
    class TaskSharedState
    {
    public:
      TaskSharedState(int uniqueId) : _uniqueId(uniqueId), _completed(false) {}

      int uniqueId() const { return _uniqueId; }
      bool completed() const { return _completed; }
      void waitForCompletion()
      {
        if (_completed)
          return;

        std::unique_lock<std::mutex> lock(_mutex);
        while (!_completed)
          _completedCondition.wait(lock);
      }
      ResultT& results()
      {
        assert(_completed);
        return _results;
      }
      void setCompleted(ResultT results)
      {
        {
          std::lock_guard<std::mutex> guard(_mutex);
          _results = std::move(results);
          _completed = true;
        }
        _completedCondition.notify_all();
      }

    private:
      int _uniqueId;
      std::atomic<bool> _completed;
      mutable std::mutex _mutex;
      std::condition_variable _completedCondition;

      ResultT _results;
    };

    /**
     *
     */
    template <typename ResultT>
    class Task
    {
    public:
      Task(std::shared_ptr<TaskSharedState<ResultT>> sharedState) : _sharedState(sharedState)
      {
        assert(_sharedState != nullptr);
      }

      int uniqueId() const { return _sharedState->uniqueId(); }
      bool completed() const { return _sharedState->completed(); }
      void waitForCompletion() { return _sharedState->waitForCompletion(); }
      ResultT& results() { return _sharedState->results(); }

    private:
      std::shared_ptr<TaskSharedState<ResultT>> _sharedState;
    };

  } // namespace op
} // namespace persistence

#endif // PERSISTENCE_OP_TASK_H
