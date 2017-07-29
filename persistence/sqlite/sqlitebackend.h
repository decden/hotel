#ifndef PERSISTENCE_SQLITE_SQLITEBACKEND_H
#define PERSISTENCE_SQLITE_SQLITEBACKEND_H

#include "persistence/datastream.h"

#include "persistence/sqlite/sqlitestorage.h"

#include "persistence/op/operations.h"
#include "persistence/op/results.h"
#include "persistence/op/task.h"

#include <boost/signals2.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <queue>

namespace persistence
{
  class ResultIntegrator;

  namespace sqlite
  {
    class SqliteBackend
    {
    public:
      SqliteBackend(const std::string& databasePath);

      op::Task<op::OperationResults> queueOperation(op::Operations operations);

      // TODO: Remove back-pointer to data source here!
      void start(persistence::ResultIntegrator& resultIntegrator);
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

      std::shared_ptr<DataStream<hotel::Hotel>> createStream(DataStreamObserver<hotel::Hotel> *observer);

    private:
      void threadMain(persistence::ResultIntegrator& dataSource);

      void executeOperation(op::OperationResults& results, op::EraseAllData&);
      void executeOperation(op::OperationResults& results, op::LoadInitialData&);
      void executeOperation(op::OperationResults& results, op::StoreNewHotel& op);
      void executeOperation(op::OperationResults& results, op::StoreNewReservation& op);
      void executeOperation(op::OperationResults& results, op::StoreNewPerson& op);
      void executeOperation(op::OperationResults& results, op::DeleteReservation& op);

      void initializeStream(DataStream<hotel::Hotel>& dataStream);

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

      std::vector<std::shared_ptr<DataStream<hotel::Hotel>>> _newHotelStreams;
      std::vector<std::shared_ptr<DataStream<hotel::Hotel>>> _activeHotelStreams;
    };

  } // namespace sqlite
} // namespace persistence

#endif // PERSISTENCE_SQLITE_SQLITESTATEMENT_H
