#ifndef PERSISTENCE_SQLITE_SQLITEBACKEND_H
#define PERSISTENCE_SQLITE_SQLITEBACKEND_H

#include "persistence/sqlite/sqlitestorage.h"

#include "persistence/op/operations.h"
#include "persistence/op/results.h"
#include "persistence/op/task.h"

#include <boost/signals2.hpp>

#include <atomic>
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

    private:
      void threadMain(persistence::ResultIntegrator& dataSource);

      op::OperationResult executeOperation(op::EraseAllData&);
      op::OperationResult executeOperation(op::LoadInitialData&);
      op::OperationResult executeOperation(op::StoreNewHotel& op);
      op::OperationResult executeOperation(op::StoreNewReservation& op);
      op::OperationResult executeOperation(op::StoreNewPerson& op);
      op::OperationResult executeOperation(op::DeleteReservation& op);

      SqliteStorage _storage;

      int _nextOperationId;

      std::thread _backendThread;
      std::atomic<bool> _quitBackendThread;
      std::condition_variable _workAvailableCondition;

      std::mutex _queueMutex;
      typedef std::shared_ptr<op::TaskSharedState<op::OperationResults>> SharedState;
      typedef std::pair<op::Operations, SharedState> QueuedOperation;
      std::queue<QueuedOperation> _operationsQueue;
      boost::signals2::signal<void(int)> _taskCompletedSignal;
    };

  } // namespace sqlite
} // namespace persistence

#endif // PERSISTENCE_SQLITE_SQLITESTATEMENT_H
