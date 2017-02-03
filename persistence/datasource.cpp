#include "persistence/datasource.h"

#include <algorithm>
#include <iostream>

namespace persistence
{
  DataSource::DataSource(const std::string& databaseFile)
      : _backend(databaseFile), _resultIntegrator(), _nextOperationId(0), _pendingOperations()
  {
    _backend.start(_resultIntegrator);
    _resultIntegrator.resultIntegratedSignal().connect([this](int id) { this->markOperationAsCompleted(id); });
    queueOperation(op::LoadInitialData());
  }

  DataSource::~DataSource()
  {
    _backend.stopAndJoin();
    _resultIntegrator.resultIntegratedSignal().disconnect_all_slots();
  }

  hotel::HotelCollection& DataSource::hotels() { return _resultIntegrator.hotels(); }
  const hotel::HotelCollection& DataSource::hotels() const { return _resultIntegrator.hotels(); }
  hotel::PlanningBoard& DataSource::planning() { return _resultIntegrator.planning(); }
  const hotel::PlanningBoard& DataSource::planning() const { return _resultIntegrator.planning(); }

  void DataSource::queueOperation(op::Operation operation)
  {
    op::Operations item;
    item.push_back(std::move(operation));
    queueOperations(std::move(item));
  }

  void DataSource::queueOperations(op::Operations operations)
  {
    op::OperationsMessage message{++_nextOperationId, std::move(operations)};
    _pendingOperations.insert(message.uniqueId);
    _backend.queueOperation(std::move(message));
  }

  void DataSource::processIntegrationQueue() { _resultIntegrator.processIntegrationQueue(); }
  void DataSource::markOperationAsCompleted(int id) { _pendingOperations.erase(id); }

} // namespace persistence
