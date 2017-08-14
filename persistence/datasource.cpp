#include "persistence/datasource.h"
#include "persistence/sqlite/sqlitebackend.h"

#include <algorithm>
#include <iostream>

namespace persistence
{
  DataSource::DataSource(const std::string& databaseFile)
  {
    _backend = std::make_unique<sqlite::SqliteBackend>(databaseFile);
    _backend->start();
  }

  DataSource::DataSource(std::unique_ptr<Backend> backend)
  {
    _backend = std::move(backend);
    _backend->start();
  }

  DataSource::~DataSource()
  {
    _backend->stopAndJoin();
  }

  op::Task<op::OperationResults> DataSource::queueOperation(op::Operation operation)
  {
    op::Operations item;
    item.push_back(std::move(operation));
    return queueOperations(std::move(item));
  }

  op::Task<op::OperationResults> DataSource::queueOperations(op::Operations operations)
  {
    return _backend->queueOperation(std::move(operations));
  }

  ChangeQueue &DataSource::changeQueue()
  {
    return _backend->changeQueue();
  }

} // namespace persistence
