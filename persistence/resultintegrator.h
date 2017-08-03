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

    // Methods used by data source:

    void addPendingTask(op::Task<op::OperationResults> task);
    size_t pendingTasksCount() const;

    void addStream(std::shared_ptr<DataStream> dataStream) { _dataStreams.push_back(std::move(dataStream)); }
    /**
     * @brief hasUninitializedStreams returns whethere there are still streams for which the initial data has not yet been set.
     * @return true if at least one stream has not yet received its initial data.
     */
    bool hasUninitializedStreams() const;

    /**
     * @brief processIntegrationQueue processes all pending changes
     */
    void processIntegrationQueue();

    // Methods used by backend

    void addStreamChange(int streamId, DataStreamChange change);

  private:
    struct DataStreamDifferential
    {
      int streamId;
      DataStreamChange change;
    };

    std::vector<std::shared_ptr<DataStream>> _dataStreams;
    std::vector<op::Task<op::OperationResults>> _tasks;

    std::mutex _changesMutex;
    std::vector<DataStreamDifferential> _streamChanges;

  };

} // namespace persistence

#endif // PERSISTENCE_RESULTINTEGRATOR_H
