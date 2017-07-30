#ifndef PERSISTENCE_RESULTINTEGRATOR_H
#define PERSISTENCE_RESULTINTEGRATOR_H

#include "persistence/datastream.h"

#include "persistence/op/operations.h"
#include "persistence/op/results.h"
#include "persistence/op/task.h"

#include "hotel/hotelcollection.h"
#include "hotel/planning.h"

#include "boost/signals2.hpp"

#include <vector>
#include <mutex>
#include <queue>

namespace persistence
{
  /**
   * @brief The ResultIntegrator class collects results from the persitency backend and applies them locally.
   */
  class ResultIntegrator
  {
  public:
    ResultIntegrator() = default;
    ~ResultIntegrator() = default;


    void addPendingTask(op::Task<op::OperationResults> task);
    size_t pendingTasksCount() const;


    template <class T>
    void addStream(std::shared_ptr<DataStream<T>> dataStream) { _dataStreams.push_back(std::move(dataStream)); }
    /**
     * @brief hasUninitializedStreams returns whethere there are still streams for which the initial data has not yet been set.
     * @return true if at least one stream has not yet received its initial data.
     */
    bool hasUninitializedStreams() const;


    void processIntegrationQueue();

  private:
    std::mutex _queueMutex;
    std::vector<DataStreamVariant> _dataStreams;
    std::vector<op::Task<op::OperationResults>> _tasks;
  };

} // namespace persistence

#endif // PERSISTENCE_RESULTINTEGRATOR_H
