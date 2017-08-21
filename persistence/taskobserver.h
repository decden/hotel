#ifndef PERSISTENCE_TASKOBSERVER_H
#define PERSISTENCE_TASKOBSERVER_H

#include "persistence/op/operations.h"

#include "extern/nlohmann_json/json.hpp"

#include <vector>

namespace persistence
{
  class Backend;

  enum class TaskResultStatus { Successful, Error };
  struct TaskResult
  {
    TaskResultStatus status;
    nlohmann::json result;
  };

  /**
   * @brief The TaskObserver class is the baseclass for all classes who want to listen for task results
   */
  class TaskObserver
  {
  public:
    virtual ~TaskObserver();

    virtual void setResults(const std::vector<TaskResult>& results) = 0;
  };

}

#endif // PERSISTENCE_TASKOBSERVER_H
