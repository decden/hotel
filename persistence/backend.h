#ifndef PERSISTENCE_BACKEND_H
#define PERSISTENCE_BACKEND_H

#include "persistence/datastream.h"
#include "persistence/task.h"

#include "persistence/op/operations.h"

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
     * @param observer the optional observer for the task result
     * @return The handle to the scheduled task. The return value can be safely ignored if no observer is provided,
     *         otherwise it has to be kept alive as long as the observer wishes to get notifications.
     */
    UniqueTaskHandle queueOperation(op::Operation operation, TaskObserver* observer = nullptr)
    {
      op::Operations ops;
      ops.emplace_back(std::move(operation));
      return queueOperations(std::move(ops), observer);
    }

    /**
     * @brief queueOperations queses an operation to be executed
     * @param operations list of operations to execute. These will be wrapped into a transaction.
     * @param observer the optional observer for the task result
     * @return The handle to the scheduled task. The return value can be safely ignored if no observer is provided,
     *         otherwise it has to be kept alive as long as the observer wishes to get notifications.
     */
    virtual UniqueTaskHandle queueOperations(op::Operations operations, TaskObserver* observer = nullptr) = 0;

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

  protected :
    friend class persistence::UniqueDataStreamHandle;
    friend class persistence::UniqueTaskHandle;

    virtual void removeStream(std::shared_ptr<persistence::DataStream> stream) = 0;
    virtual void removeTask(std::shared_ptr<persistence::Task> task) = 0;
  };
} // namespace persistence

#endif // PERSISTENCE_BACKEND_H
