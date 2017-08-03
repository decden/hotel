#ifndef PERSISTENCE_SQLITE_SQLITEBACKEND_H
#define PERSISTENCE_SQLITE_SQLITEBACKEND_H

#include "persistence/datastream.h"

#include "persistence/sqlite/sqlitestorage.h"

#include "persistence/op/operations.h"
#include "persistence/op/results.h"
#include "persistence/op/task.h"

#include "extern/nlohmann_json/json.hpp"

#include <boost/signals2.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <queue>
#include <functional>

namespace persistence
{
  class ResultIntegrator;

  namespace detail {
    class DataStreamManager
    {
    public:
      /**
       * @note addNewStream must be synchronized!
       * @see collectNewStreams
       */
      void addNewStream(const std::shared_ptr<DataStream>& stream) { _newStreams.push_back(stream); }

      /**
       * @brief The purpose of this method is to move all new streams into the uninitializedStreams list
       * When this method is called it has to be ensured that no one accesses either the newStreams or
       * uninitializedStreams list.
       *
       * @note Before calling this method again, initialize() has to be called.
       * @note initialize() can only be called on the worker thread while no other thread is accessing _newStreams.
       *
       * @return True if there are uninitialized streams to process
       */
      bool collectNewStreams();

      /**
       * @brief Initializes all new streams
       * This function has to be called after collectNewStreams()
       * @note initialize() can only be called on the worker thread!
       */
      template <class Func>
      void initialize(Func initializerFunction);

      /**
       * @brief Calls func for each data stream in the active queue
       */
      template <class T, class Func>
      void foreachStream(Func func);

      template <class Func>
      void foreachStream(StreamableType type, Func func);

      virtual void addItems(ResultIntegrator& integrator, StreamableType type, const std::string& subtype, const StreamableItems items);
      virtual void removeItems(ResultIntegrator& integrator, StreamableType type, const std::string& subtype, std::vector<int> ids);
      virtual void clear(ResultIntegrator& integrator, StreamableType type, const std::string& subtype);

    private:
      // Access to this list has to be synchronized by the backend
      std::vector<std::shared_ptr<DataStream>> _newStreams;
      // The following two lists can be operated on by the worker thread without lock!
      std::vector<std::shared_ptr<DataStream>> _uninitializedStreams;
      std::vector<std::shared_ptr<DataStream>> _activeStreams;
    };
  }

  namespace sqlite
  {
    /**
     * @brief The SqliteBackend class is the sqlite data backend for the application
     *
     * This particular backend will create its own worker thread, on which all data operations will be executed.
     */
    class SqliteBackend
    {
    public:
      SqliteBackend(const std::string& databasePath);

      op::Task<op::OperationResults> queueOperation(op::Operations operations);

      void start(ResultIntegrator& integrator);
      void stopAndJoin();

      /**
       * @brief taskCompletedSignal returns the signal which is triggered when operations have been completed and results are available
       * @note The signal is not called on the main thread, but on the backend worker thread
       */
      boost::signals2::signal<void(int)>& taskCompletedSignal() { return _taskCompletedSignal; }
      /**
       * @brief streamsUpdatedSignal returns the signal which is triggered when new data has been made available to a stream
       * @note The signal is not called on the main thread, but on the backend worker thread
       */
      boost::signals2::signal<void()>& streamsUpdatedSignal() { return _streamsUpdatedSignal; }

      /**
       * @brief Creates a new stream which connects the given observer to the given service endpoint
       * @param observer The observer which will listen to the stream
       * @param service The name of the backend service to connect to
       * @param options Additional parameters for the service endpoint
       * @return The shared state for the data stream
       */
      template <class T>
      std::shared_ptr<DataStream> createStream(DataStreamObserverTyped<T> *observer, const std::string& service,
                                               const nlohmann::json& options)
      {
        std::unique_lock<std::mutex> lock(_queueMutex);

        auto sharedState = makeStream(DataStream::GetStreamTypeFor<T>(), service, options);
        if (sharedState == nullptr)
        {
          sharedState = std::make_shared<DataStream>(StreamableType::NullStream);
          std::cerr << "Unknown data stream for type \"" << typeid(T).name() << "\" and service \"" << service << "\"" << std::endl;
        }

        sharedState->connect(_nextStreamId++, observer);
        _dataStreams.addNewStream(sharedState);
        lock.unlock();

        _workAvailableCondition.notify_one();

        return sharedState;
      }

    private:
      void threadMain(ResultIntegrator& integrator);

      std::shared_ptr<DataStream> makeStream(StreamableType type, const std::string& service, const nlohmann::json& json);

      op::OperationResult executeOperation(ResultIntegrator& integrator, op::EraseAllData&);
      op::OperationResult executeOperation(ResultIntegrator& integrator, op::StoreNewHotel& op);
      op::OperationResult executeOperation(ResultIntegrator& integrator, op::StoreNewReservation& op);
      op::OperationResult executeOperation(ResultIntegrator& integrator, op::StoreNewPerson& op);
      op::OperationResult executeOperation(ResultIntegrator& integrator, op::DeleteReservation& op);

      template <class T>
      void initializeStreamTyped(const DataStream& dataStream, ResultIntegrator& integrator);
      void initializeStream(const DataStream& dataStream, ResultIntegrator& integrator);

      SqliteStorage _storage;

      int _nextOperationId;
      int _nextStreamId;

      std::thread _backendThread;
      std::atomic<bool> _quitBackendThread;
      std::condition_variable _workAvailableCondition;

      std::mutex _queueMutex;
      typedef std::shared_ptr<op::TaskSharedState<op::OperationResults>> SharedState;
      typedef std::pair<op::Operations, SharedState> QueuedOperation;
      std::vector<QueuedOperation> _operationsQueue;
      boost::signals2::signal<void(int)> _taskCompletedSignal;
      boost::signals2::signal<void()> _streamsUpdatedSignal;

      detail::DataStreamManager _dataStreams;
    };

  } // namespace sqlite
} // namespace persistence

#endif // PERSISTENCE_SQLITE_SQLITESTATEMENT_H
