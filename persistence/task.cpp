#include "persistence/task.h"
#include "persistence/backend.h"

#include <algorithm>
#include <iostream>

namespace persistence
{

  Task::~Task() {}

  void Task::connect(TaskObserver *observer)
  {
    assert(_observer == nullptr);
    _observer = observer;
  }

  void Task::setResults(const std::vector<TaskResult>& results)
  {
    assert(!_isCompleted);
    if (_observer)
      _observer->setResults(results);
    _isCompleted = true;
  }

  UniqueTaskHandle::UniqueTaskHandle(UniqueTaskHandle&& that)
      : _backend(that._backend), _task(std::move(that._task))
  {
    that._backend = nullptr;
  }

  UniqueTaskHandle& UniqueTaskHandle::operator=(UniqueTaskHandle&& that)
  {
    reset();
    _backend = that._backend;
    that._backend = nullptr;
    _task = std::move(that._task);
  }

  UniqueTaskHandle::~UniqueTaskHandle() { reset(); }

  void UniqueTaskHandle::reset()
  {
    if (_task)
    {
      _task->disconnect();
      _backend->removeTask(_task);
    }
    _task = nullptr;
    _backend = nullptr;
  }

} // namespace persistence
