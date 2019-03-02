#ifndef PERSISTENCE_SQLITE_SQLITEBACKEND_H
#define PERSISTENCE_SQLITE_SQLITEBACKEND_H

#include "persistence/sqlite/sqlitestorage.h"

#include "persistence/backend.h"
#include "persistence/datastream.h"

#include "persistence/changequeue.h"
#include "persistence/op/operations.h"

#include "fas/threadedexecutor.h"

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
      virtual ~DataStreamHandler() = default;
      virtual void initialize(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue, sqlite::SqliteStorage& storage) = 0;
      virtual void addItems(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue, const StreamableItems& items) = 0;
      virtual void updateItems(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue, const StreamableItems& items) = 0;
      virtual void removeItems(DataStream& stream, std::vector<DataStreamDifferential>& ChangeQueue, const std::vector<int> ids) = 0;
      virtual void clear(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue) = 0;
    };

    class DataStreamManager
    {
    public:
      DataStreamManager();
      virtual ~DataStreamManager() = default;

      /**
       * @note addNewStream must be synchronized!
       * @see collectNewStreams
       */
      void addNewStream(const std::shared_ptr<DataStream>& stream);

      void removeStream(const std::shared_ptr<DataStream>& stream);

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

      bool hasUninitializedStreams() const
      {
        std::lock_guard<std::mutex> lock(_streamMutex);
        return !_uninitializedStreams.empty();
      }

      virtual void addItems(std::vector<DataStreamDifferential>& changeQueue, StreamableType type, const StreamableItems items);
      virtual void updateItems(std::vector<DataStreamDifferential>& changeQueue, StreamableType type, const StreamableItems items);
      virtual void removeItems(std::vector<DataStreamDifferential>& changeQueue, StreamableType type, std::vector<int> ids);
      virtual void clear(std::vector<DataStreamDifferential>& changeQueue, StreamableType type);

    private:
      DataStreamHandler* findHandler(const DataStream& stream);

      typedef std::tuple<StreamableType, std::string> HandlerKey;
      std::map<HandlerKey, std::unique_ptr<DataStreamHandler>> _streamHandlers;

      mutable std::mutex _streamMutex;
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
    class SqliteBackend final : public Backend
    {
    public:
      SqliteBackend(const std::string& databasePath);
      virtual ~SqliteBackend();

      virtual fas::Future<std::vector<TaskResult>> queueOperations(op::Operations operations) override;

      virtual persistence::UniqueDataStreamHandle createStream(DataStreamObserver* observer, StreamableType type,
                                                               const std::string& service,
                                                               const nlohmann::json& options) override;

      ChangeQueue& changeQueue() override { return _changeQueue; }

    protected:
      virtual void removeStream(std::shared_ptr<persistence::DataStream> stream) override;

    private:
      void start();
      void stopAndJoin();
      void threadMain();

      TaskResult executeOperation(op::EraseAllData&, std::vector<DataStreamDifferential>& streamChanges);
      TaskResult executeOperation(op::StoreNew& op, std::vector<DataStreamDifferential>& streamChanges);
      TaskResult executeOperation(op::Update& op, std::vector<DataStreamDifferential>& streamChanges);
      TaskResult executeOperation(op::Delete& op, std::vector<DataStreamDifferential>& streamChanges);

      TaskResult executeStoreNew(hotel::Hotel& hotel, std::vector<DataStreamDifferential>& streamChanges);
      TaskResult executeStoreNew(hotel::Reservation& reservation, std::vector<DataStreamDifferential>& streamChanges);
      TaskResult executeStoreNew(hotel::Person& person, std::vector<DataStreamDifferential>& streamChanges);

      void executeUpdate(const hotel::Hotel& hotel, std::vector<DataStreamDifferential>& streamChanges);
      void executeUpdate(const hotel::Reservation& reservation, std::vector<DataStreamDifferential>& streamChanges);
      void executeUpdate(const hotel::Person& person, std::vector<DataStreamDifferential>& streamChanges);

      SqliteStorage _storage;
      ChangeQueue _changeQueue;

      int _nextOperationId;
      int _nextStreamId;

      std::thread _backendThread;
      std::atomic<bool> _quitBackendThread;
      std::condition_variable _workAvailableCondition;

      std::mutex _queueMutex;
      typedef std::pair<op::Operations, fas::Promise<std::vector<TaskResult>>> QueuedOperation;
      std::vector<QueuedOperation> _operationsQueue;

      detail::DataStreamManager _dataStreams;
    };

  } // namespace sqlite
} // namespace persistence

#endif // PERSISTENCE_SQLITE_SQLITESTATEMENT_H
