#ifndef PERSISTENCE_SQLITE_SQLITEBACKEND_H
#define PERSISTENCE_SQLITE_SQLITEBACKEND_H

#include "persistence/sqlite/sqlitestorage.h"

#include "persistence/op/operations.h"
#include "persistence/op/results.h"

#include <string>

namespace persistence
{
  namespace sqlite
  {
    class SqliteBackend
    {
    public:
      SqliteBackend(const std::string& databasePath);

      op::OperationResults execute(op::Operations operationBlock);

    private:
      op::OperationResult executeOperation(op::EraseAllData&);
      op::OperationResult executeOperation(op::LoadInitialData&);
      op::OperationResult executeOperation(op::StoreNewHotel& op);
      op::OperationResult executeOperation(op::StoreNewReservation& op);
      op::OperationResult executeOperation(op::StoreNewPerson& op);

      SqliteStorage _storage;
    };

  } // namespace sqlite
} // namespace persistence

#endif // PERSISTENCE_SQLITE_SQLITESTATEMENT_H
