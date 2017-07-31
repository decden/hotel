#include "persistence/resultintegrator.h"

#include <algorithm>

namespace persistence
{
  void ResultIntegrator::processIntegrationQueue()
  {
    std::unique_lock<std::mutex> lock(_queueMutex);

    // Remove invalid streams (streams which no longer have a listener)
    _dataStreams.erase(std::remove_if(_dataStreams.begin(), _dataStreams.end(), [](auto& stream) {
      return !stream->isValid();
    }), _dataStreams.end());
    // Apply changes to streams
    for (auto& stream : _dataStreams)
      stream->integrateChanges();

    // Move all of the completed tasks to the end of the list
    auto readyBegin = std::stable_partition(begin(_tasks), end(_tasks),
                                            [](auto& task) { return !task.completed(); });
    // Erase all completed tasks
    _tasks.erase(readyBegin, end(_tasks));
  }

  void ResultIntegrator::addPendingTask(op::Task<op::OperationResults> task)
  {
    std::unique_lock<std::mutex> lock(_queueMutex);
    _tasks.push_back(std::move(task));
  }

  size_t ResultIntegrator::pendingTasksCount() const
  {
    return _tasks.size();
  }

  bool ResultIntegrator::hasUninitializedStreams() const
  {
    return std::any_of(_dataStreams.begin(), _dataStreams.end(), [](const std::shared_ptr<DataStream>& stream) {
      return !stream->isInitialized();
    });
  }

} // namespace persistence
