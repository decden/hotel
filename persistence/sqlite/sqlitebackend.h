#ifndef PERSISTENCE_SQLITE_SQLITEBACKEND_H
#define PERSISTENCE_SQLITE_SQLITEBACKEND_H

#include "persistence/datastream.h"

#include "persistence/sqlite/sqlitestorage.h"

#include "persistence/changequeue.h"
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
  class ChangeQueue;

  namespace detail {
    class DataStreamHandler
    {
    public:
      virtual ~DataStreamHandler() {}
      virtual void initialize(DataStream& stream, ChangeQueue& changeQueue, sqlite::SqliteStorage& storage) = 0;
      virtual void addItems(DataStream& stream, ChangeQueue& changeQueues, const StreamableItems& items) = 0;
      virtual void removeItems(DataStream& stream, ChangeQueue& ChangeQueue, const std::vector<int> ids) = 0;
      virtual void clear(DataStream& stream, ChangeQueue& changeQueue) = 0;
    };

    class DataStreamManager
    {
    public:
      DataStreamManager();

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
      void initialize(ChangeQueue& changeQueue, sqlite::SqliteStorage& storage);

      /**
       * @brief Calls func for each data stream in the active queue
       */
      template <class T, class Func>
      void foreachStream(Func func);

      template <class Func>
      void foreachStream(StreamableType type, Func func);

      virtual void addItems(ChangeQueue& changeQueue, StreamableType type, const StreamableItems items);
      virtual void removeItems(ChangeQueue& changeQueue, StreamableType type, std::vector<int> ids);
      virtual void clear(ChangeQueue& changeQueue, StreamableType type);

    private:
      DataStreamHandler* findHandler(const DataStream& stream);

      typedef std::tuple<StreamableType, std::string> HandlerKey;
      std::map<HandlerKey, std::unique_ptr<DataStreamHandler>> _streamHandlers;

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

      void start();
      void stopAndJoin();

      ChangeQueue& getChangeQueue() { return _changeQueue; }

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
          sharedState = std::make_shared<DataStream>(StreamableType::NullStream, "", nlohmann::json());
          std::cerr << "Unknown data stream for type \"" << typeid(T).name() << "\" and service \"" << service << "\"" << std::endl;
        }

        sharedState->connect(_nextStreamId++, observer);
        _changeQueue.addStream(sharedState);
        _dataStreams.addNewStream(sharedState);
        lock.unlock();

        _workAvailableCondition.notify_one();

        return sharedState;
      }

    private:
      void threadMain();

      std::shared_ptr<DataStream> makeStream(StreamableType type, const std::string& service, const nlohmann::json& options);

      op::OperationResult executeOperation(op::EraseAllData&);
      op::OperationResult executeOperation(op::StoreNewHotel& op);
      op::OperationResult executeOperation(op::StoreNewReservation& op);
      op::OperationResult executeOperation(op::StoreNewPerson& op);
      op::OperationResult executeOperation(op::DeleteReservation& op);

      SqliteStorage _storage;
      ChangeQueue _changeQueue;

      int _nextOperationId;
      int _nextStreamId;

      std::thread _backendThread;
      std::atomic<bool> _quitBackendThread;
      std::condition_variable _workAvailableCondition;

      std::mutex _queueMutex;
      typedef std::shared_ptr<op::TaskSharedState<op::OperationResults>> SharedState;
      typedef std::pair<op::Operations, SharedState> QueuedOperation;
      std::vector<QueuedOperation> _operationsQueue;

      detail::DataStreamManager _dataStreams;
    };

  } // namespace sqlite
} // namespace persistence

#endif // PERSISTENCE_SQLITE_SQLITESTATEMENT_H
