#ifndef PERSISTENCE_TASKRESULT_H
#define PERSISTENCE_TASKRESULT_H

#include "extern/nlohmann_json/json.hpp"

namespace persistence
{
  enum class TaskResultStatus { Successful, Error };
  struct TaskResult
  {
    TaskResultStatus status;
    nlohmann::json result;
  };
}

#endif // PERSISTENCE_TASKRESULT_H
