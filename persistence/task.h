#ifndef PERSISTENCE_TASK_H
#define PERSISTENCE_TASK_H

#include "persistence/taskobserver.h"

#include <algorithm>
#include <memory>
#include <vector>

namespace persistence
{
  class Backend;

  /**
   * @brief Writable backend for data stream
   */
  class Task
  {
  public:
    Task(int taskId) : _taskId(taskId), _isCompleted(false), _observer(nullptr) {}
    virtual ~Task();

    void connect(TaskObserver* observer);

    //! Returns the unique ID for this task
    int taskId() const { return _taskId; }

    //! Returns true if there is still an observer listening on this task
    bool isValid() const { return _observer != nullptr; }
    //! Returns true if the task has been completed
    bool isCompleted() const { return _isCompleted; }
    //! Dissociates the task from the observer.
    void disconnect() { _observer = nullptr; }

    void setResults(const std::vector<TaskResult>& results);

  private:
    int _taskId;
    bool _isCompleted;
    TaskObserver* _observer;
  };

  /**
   * @brief The UniqueTaskHandle class is a handle for a task observer connection
   *
   * When this handle is destroyed, the associated task observer will no longer get any change notifications.
   */
  class UniqueTaskHandle
  {
  public:
    UniqueTaskHandle() : _backend(nullptr), _task(nullptr) {}
    UniqueTaskHandle(Backend* backend, std::shared_ptr<Task> task) : _backend(backend), _task(task) {}

    UniqueTaskHandle(const UniqueTaskHandle& that) = delete;
    UniqueTaskHandle& operator=(const UniqueTaskHandle& that) = delete;

    UniqueTaskHandle(UniqueTaskHandle&& that) = default;
    UniqueTaskHandle& operator=(UniqueTaskHandle&& that) = default;
    ~UniqueTaskHandle();

    Task* task() { return _task.get(); }

  private:
    Backend* _backend;
    std::shared_ptr<Task> _task;
  };

}

#endif // PERSISTENCE_TASK_H
