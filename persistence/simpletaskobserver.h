#ifndef PERSISTENCE_SIMPLETASKOBSERVER_H
#define PERSISTENCE_SIMPLETASKOBSERVER_H

#include "persistence/backend.h"
#include "persistence/task.h"
#include "persistence/taskobserver.h"


namespace persistence
{
  /**
   * @brief Simple implementation of TaskObserver which stores the results internally
   */
  class SimpleTaskObserver final : public TaskObserver
  {
  public:
    virtual ~SimpleTaskObserver() {}
    SimpleTaskObserver(Backend& backend, persistence::op::Operations ops);
    SimpleTaskObserver(Backend& backend, persistence::op::Operation op);

    virtual void setResults(const std::vector<TaskResult>& results) override;

    const std::vector<TaskResult>& results();
    bool isCompleted() const;
    Task* task() { return _handle.task(); }

  private:
    UniqueTaskHandle _handle;
    std::vector<TaskResult> _results;
  };
}

#endif // PERSISTENCE_SIMPLETASKOBSERVER_H
