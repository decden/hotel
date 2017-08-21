#include "persistence/changequeue.h"

#include <algorithm>

namespace persistence
{
  void ChangeQueue::addStream(std::shared_ptr<DataStream> dataStream) { _dataStreams.push_back(std::move(dataStream)); }
  void ChangeQueue::addTask(std::shared_ptr<Task> task) { _tasks.push_back(std::move(task)); }

  bool ChangeQueue::hasUninitializedStreams() const
  {
    return std::any_of(_dataStreams.begin(), _dataStreams.end(),
                       [](const std::shared_ptr<DataStream>& stream) { return !stream->isInitialized(); });
  }

  void ChangeQueue::applyStreamChanges()
  {
    // Remove invalid streams (streams which no longer have a listener)
    _dataStreams.erase(std::remove_if(_dataStreams.begin(), _dataStreams.end(), [](auto& stream) {
      return !stream->isValid();
    }), _dataStreams.end());

    // Apply all of the strema changes
    std::vector<DataStreamDifferential> changes;
    std::unique_lock<std::mutex> lock(_streamChangesMutex);
    std::swap(_streamChangeQueue, changes);
    lock.unlock();

    for (auto& change: changes)
    {
      auto it = std::find_if(_dataStreams.begin(),
                             _dataStreams.end(), [&](std::shared_ptr<DataStream>& ds) { return ds->streamId() == change.streamId; });
      if (it != _dataStreams.end())
        (*it)->applyChange(change.change);
    }
  }

  void ChangeQueue::applyTaskChanges()
  {
    // Remove invalid tasks
    _tasks.erase(std::remove_if(_tasks.begin(), _tasks.end(), [](auto& task) {
      return !task->isValid() && task->isCompleted();
    }), _tasks.end());

    // Apply all of the task changes
    std::vector<TaskDifferential> changes;
    std::unique_lock<std::mutex> lock(_completedTasksMutex);
    std::swap(_taskChangeQueue, changes);
    lock.unlock();

    for (auto& change : changes)
    {
      auto it = std::find_if(_tasks.begin(), _tasks.end(),
                             [&](std::shared_ptr<Task>& task) { return task->taskId() == change.taskId; });
      if (it != _tasks.end())
        (*it)->setResults(std::move(change.results));
    }
  }

  void ChangeQueue::addTaskChange(int taskId, std::vector<TaskResult> results)
  {
    std::unique_lock<std::mutex> lock(_completedTasksMutex);
    _taskChangeQueue.push_back({taskId, std::move(results)});
    lock.unlock();
    _taskCompletedSignal();
  }

  void ChangeQueue::addStreamChange(int streamId, DataStreamChange change)
  {
    std::unique_lock<std::mutex> lock(_streamChangesMutex);
    _streamChangeQueue.push_back({streamId, std::move(change)});
    lock.unlock();
    _streamChangesAvailableSignal();
  }

  boost::signals2::connection ChangeQueue::connectToTaskCompletedSignal(boost::signals2::slot<void()> slot)
  {
    return _taskCompletedSignal.connect(slot);
  }

  boost::signals2::connection ChangeQueue::connectToStreamChangesAvailableSignal(boost::signals2::slot<void ()> slot)
  {
    return _streamChangesAvailableSignal.connect(slot);
  }

} // namespace persistence
