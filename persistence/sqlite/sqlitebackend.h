#ifndef PERSISTENCE_SQLITE_SQLITEBACKEND_H
#define PERSISTENCE_SQLITE_SQLITEBACKEND_H

#include "persistence/sqlite/sqlitestorage.h"

#include "persistence/op/operations.h"
#include "persistence/op/results.h"

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <queue>

namespace persistence
{
  class DataSource;

  namespace sqlite
  {
    class SqliteBackend
    {
    public:
      SqliteBackend(const std::string& databasePath);

      void queueOperation(op::OperationsMessage operationsMessage);

      // TODO: Remove back-pointer to data source here!
      void start(persistence::DataSource& dataSource);
      void stopAndJoin();

    private:
      void threadMain(persistence::DataSource& dataSource);

      op::OperationResult executeOperation(op::EraseAllData&);
      op::OperationResult executeOperation(op::LoadInitialData&);
      op::OperationResult executeOperation(op::StoreNewHotel& op);
      op::OperationResult executeOperation(op::StoreNewReservation& op);
      op::OperationResult executeOperation(op::StoreNewPerson& op);
      op::OperationResult executeOperation(op::DeleteReservation& op);

      SqliteStorage _storage;

      std::thread _backendThread;
      std::atomic<bool> _quitBackendThread;
      std::condition_variable _workAvailableCondition;

      std::mutex _queueMutex;
      std::queue<op::OperationsMessage> _operationsQueue;
    };

  } // namespace sqlite
} // namespace persistence

#endif // PERSISTENCE_SQLITE_SQLITESTATEMENT_H
