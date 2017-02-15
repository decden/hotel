#include "persistence/datasource.h"

#include <algorithm>
#include <iostream>

namespace persistence
{
  DataSource::DataSource(const std::string& databaseFile)
      : _backend(databaseFile), _resultIntegrator(), _nextOperationId(0)
  {
    _backend.start(_resultIntegrator);
    queueOperation(op::LoadInitialData());
  }

  DataSource::~DataSource()
  {
    _backend.stopAndJoin();
  }

  hotel::HotelCollection& DataSource::hotels() { return _resultIntegrator.hotels(); }
  const hotel::HotelCollection& DataSource::hotels() const { return _resultIntegrator.hotels(); }
  hotel::PlanningBoard& DataSource::planning() { return _resultIntegrator.planning(); }
  const hotel::PlanningBoard& DataSource::planning() const { return _resultIntegrator.planning(); }

  op::Task<op::OperationResults> DataSource::queueOperation(op::Operation operation)
  {
    op::Operations item;
    item.push_back(std::move(operation));
    return queueOperations(std::move(item));
  }

  op::Task<op::OperationResults> DataSource::queueOperations(op::Operations operations)
  {
    auto task = _backend.queueOperation(std::move(operations));
    _resultIntegrator.addPendingOperation(task);
    return task;
  }

  void DataSource::processIntegrationQueue()
  {
    _resultIntegrator.processIntegrationQueue();
  }

  size_t DataSource::pendingOperationsCount() const
  {
    return _resultIntegrator.pendingOperationsCount();
  }

} // namespace persistence
