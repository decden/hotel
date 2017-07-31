#ifndef PERSISTENCE_DATASOURCE_H
#define PERSISTENCE_DATASOURCE_H

#include "persistence/datastream.h"
#include "persistence/op/operations.h"
#include "persistence/op/results.h"
#include "persistence/resultintegrator.h"
#include "persistence/sqlite/sqlitebackend.h"

#include "hotel/planning.h"

#include "extern/nlohmann_json/json.hpp"

#include "boost/signals2.hpp"

#include <memory>
#include <mutex>
#include <queue>
#include <set>

namespace persistence
{

  /**
   * @brief The DataSource class handles all writing and reading access to the data.
   */
  class DataSource
  {
  public:
    DataSource(const std::string& databaseFile);
    ~DataSource();

    /**
     * @brief queueOperation queues a given operation to perform on the data
     * @param operation The operation to perform
     *
     * @note The operation might be performed immediately or queued for later execution
     */
    op::Task<op::OperationResults> queueOperation(op::Operation operation);

    /**
     * @brief queueOperations queues multiple operations
     * The operations are executed together under a transaction if possible.
     * @param operations List of operations to perform.
     */
    op::Task<op::OperationResults> queueOperations(op::Operations operations);

    template <class T>
    UniqueDataStreamHandle connectToStream(DataStreamObserverTyped<T>* observer)
    {
      // Connect to default service with default options
      return connectToStream(observer, "", {});
    }

    template <class T>
    UniqueDataStreamHandle connectToStream(DataStreamObserverTyped<T> *observer, const std::string& service, const nlohmann::json& options)
    {
      auto stream = _backend.createStream(observer, service, options);
      _resultIntegrator.addStream(stream);
      return UniqueDataStreamHandle(stream);
    }

    void processIntegrationQueue();

    /**
     * @brief hasPendingTasks returns whether there any queued operations are still being processed.
     * Note that his method does only consider the tasks which have been queued with the queueOperation() or
     * queueOperations() method.
     * @return false if some tasks are still being processed, true otherwise.
     */
    bool hasPendingTasks() const;

    /**
     * @brief hasUninitializedStreams returns whethere there are still streams for which the initial data has not yet been set.
     * @return true if at least one stream has not yet received its initial data.
     */
    bool hasUninitializedStreams() const;

    /**
     * @brief taskCompletedSignal returns the signal which is triggered when new results are waiting to be integrated
     * @note The signal is not called on the main thread, but on the backend worker thread
     */
    boost::signals2::signal<void(int)>& taskCompletedSignal() { return _backend.taskCompletedSignal(); }
    /**
     * @brief streamsUpdatedSignal returns the signal which is triggered when new data has been made available to a stream
     * @note The signal is not called on the main thread, but on the backend worker thread
     */
    boost::signals2::signal<void()>& streamsUpdatedSignal() { return _backend.streamsUpdatedSignal(); }

  private:
    // Backing store and result integrator
    persistence::sqlite::SqliteBackend _backend;
    persistence::ResultIntegrator _resultIntegrator;

    std::vector<op::Task<op::OperationResults>> _pendingTasks;
  };

} // namespace persistence

#endif // PERSISTENCE_DATASOURCE_H
