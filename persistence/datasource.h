#ifndef PERSISTENCE_DATASOURCE_H
#define PERSISTENCE_DATASOURCE_H

#include "persistence/changequeue.h"
#include "persistence/datastream.h"
#include "persistence/op/operations.h"
#include "persistence/op/results.h"
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
      return UniqueDataStreamHandle(_backend.createStream(observer, service, options));
    }

    /**
     * @brief getChangeQueue returns the queue of changes which are produced by this data source.
     * @return reference to the backend's change queue
     */
    ChangeQueue &changeQueue();

  private:
    persistence::sqlite::SqliteBackend _backend;
  };

} // namespace persistence

#endif // PERSISTENCE_DATASOURCE_H
