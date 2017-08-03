#include "persistence/resultintegrator.h"

#include <algorithm>

namespace persistence
{
  void ResultIntegrator::processIntegrationQueue()
  {
    // Move all of the completed tasks to the end of the list
    auto readyBegin = std::stable_partition(begin(_tasks), end(_tasks),
                                            [](auto& task) { return !task.completed(); });
    // Erase all completed tasks
    _tasks.erase(readyBegin, end(_tasks));

    // Remove invalid streams (streams which no longer have a listener)
    _dataStreams.erase(std::remove_if(_dataStreams.begin(), _dataStreams.end(), [](auto& stream) {
      return !stream->isValid();
    }), _dataStreams.end());

    // Apply all of the strema changes
    std::vector<DataStreamDifferential> changes;
    std::unique_lock<std::mutex> lock(_changesMutex);
    std::swap(_streamChanges, changes);
    lock.unlock();
    for (auto& change: changes)
    {
      auto it = std::find_if(_dataStreams.begin(),
                             _dataStreams.end(), [&](std::shared_ptr<DataStream>& ds) { return ds->streamId() == change.streamId; });
      if (it != _dataStreams.end())
        (*it)->applyChange(change.change);
    }
  }


  void ResultIntegrator::addPendingTask(op::Task<op::OperationResults> task)
  {
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

  void ResultIntegrator::addStreamChange(int streamId, DataStreamChange change)
  {
    std::unique_lock<std::mutex> lock(_changesMutex);
    _streamChanges.push_back({streamId, std::move(change)});
  }

} // namespace persistence
