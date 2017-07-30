#include "persistence/datasource.h"

#include <algorithm>
#include <iostream>

namespace persistence
{
  DataSource::DataSource(const std::string& databaseFile)
      : _backend(databaseFile), _resultIntegrator()
  {
    _backend.start(_resultIntegrator);
  }

  DataSource::~DataSource()
  {
    _backend.stopAndJoin();
  }

  op::Task<op::OperationResults> DataSource::queueOperation(op::Operation operation)
  {
    op::Operations item;
    item.push_back(std::move(operation));
    return queueOperations(std::move(item));
  }

  op::Task<op::OperationResults> DataSource::queueOperations(op::Operations operations)
  {
    auto task = _backend.queueOperation(std::move(operations));
    _resultIntegrator.addPendingTask(task);
    _pendingTasks.push_back(task);
    return task;
  }

  void DataSource::processIntegrationQueue()
  {
    _resultIntegrator.processIntegrationQueue();

    // Remove completed tasks
    _pendingTasks.erase(std::remove_if(_pendingTasks.begin(), _pendingTasks.end(),
      [](const op::Task<op::OperationResults>& task) {
        return task.completed();
    }), _pendingTasks.end());
  }

  bool DataSource::hasPendingTasks() const
  {
    return !_pendingTasks.empty();
  }

} // namespace persistence
