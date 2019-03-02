#ifndef PERSISTENCE_BACKEND_H
#define PERSISTENCE_BACKEND_H

#include "persistence/datastream.h"
#include "persistence/op/operations.h"
#include "persistence/taskresult.h"

#include "fas/future.h"

#include "extern/nlohmann_json/json.hpp"

#include <memory>
#include <string>

namespace persistence
{
  class ChangeQueue;

  /**
   * @brief The Backend class is the interface which must be iplemented for each backend
   *
   * A backend handles the execuion of tasks and the creation of data streams.
   */
  class Backend
  {
  public:
    Backend();
    virtual ~Backend();

    /**
     * @brief changeQueue returns the queue to which all changes are pushed
     */
    virtual ChangeQueue& changeQueue() = 0;

    /**
     * @brief queueOperation queses a single operation to be executed
     * @param operation the operation to queue.
     * @return A future which will eventually contain the result of the operation
     */
    fas::Future<std::vector<TaskResult>> queueOperation(op::Operation operation)
    {
      op::Operations ops;
      ops.emplace_back(std::move(operation));
      return queueOperations(std::move(ops));
    }

    /**
     * @brief queueOperations queses an operation to be executed
     * @param operations list of operations to execute. These will be wrapped into a transaction.
     * @return A future which will eventually contain the result of the operations
     */
    virtual fas::Future<std::vector<TaskResult>> queueOperations(op::Operations operations) = 0;

    /**
     * @brief Creates a new stream which connects the given observer to the given service endpoint
     * @param observer The observer which will listen to the stream
     * @param service The name of the backend service to connect to
     * @param options Additional parameters for the service endpoint
     * @return A handle to the created stream. The stream will be open until the handle is destroyed.
     */
    template <class T>
    persistence::UniqueDataStreamHandle createStreamTyped(DataStreamObserverTyped<T>* observer,
                                                          const std::string& service = "",
                                                          const nlohmann::json& options = {})
    {
      return createStream(observer, DataStream::GetStreamTypeFor<T>(), service, options);
    }

    /**
     * @brief Creates a new stream which connects the given observer to the given service endpoint
     * @param observer The observer which will listen to the stream
     * @param type The data type of the stream
     * @param service The name of the backend service to connect to
     * @param options Additional parameters for the service endpoint
     * @return A handle to the created stream. The stream will be open until the handle is destroyed.
     */
    virtual persistence::UniqueDataStreamHandle createStream(DataStreamObserver* observer, StreamableType type,
                                                             const std::string& service,
                                                             const nlohmann::json& options) = 0;

  protected:
    friend class persistence::UniqueDataStreamHandle;

    virtual void removeStream(std::shared_ptr<persistence::DataStream> stream) = 0;
  };
} // namespace persistence

#endif // PERSISTENCE_BACKEND_H
