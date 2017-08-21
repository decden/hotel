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

  UniqueTaskHandle::~UniqueTaskHandle()
  {
    if (_task)
    {
      _backend->removeTask(_task);
      _task->disconnect();
    }
  }

} // namespace persistence
