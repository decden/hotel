#include "persistence/simpletaskobserver.h"

namespace persistence
{

  SimpleTaskObserver::SimpleTaskObserver(Backend& backend, op::Operations ops)
  {
    _handle = backend.queueOperations(std::move(ops), this);
  }

  SimpleTaskObserver::SimpleTaskObserver(Backend& backend, op::Operation op)
  {
    _handle = backend.queueOperation(std::move(op), this);
  }

  void SimpleTaskObserver::setResults(const std::vector<TaskResult>& results)
  {
    assert(_results.empty());
    _results = results;
  }

  const std::vector<TaskResult>& SimpleTaskObserver::results() { return _results; }

  bool SimpleTaskObserver::isCompleted() const { return !_results.empty(); }

} // namespace persistence
