#include "persistence/datasource.h"

#include <algorithm>
#include <iostream>

namespace persistence
{
  DataSource::DataSource(const std::string& databaseFile) : _backend(databaseFile)
  {
    queueOperation(op::LoadInitialData());
  }

  hotel::HotelCollection& DataSource::hotels() { return _hotels; }
  const hotel::HotelCollection& DataSource::hotels() const { return _hotels; }
  hotel::PlanningBoard& DataSource::planning() { return _planning; }
  const hotel::PlanningBoard& DataSource::planning() const { return _planning; }

  void DataSource::queueOperation(op::Operation operation)
  {
    op::Operations item;
    item.push_back(std::move(operation));
    _operationsQueue.push_back(std::move(item));
    processQueue();
    processIntegrationQueue();
  }

  void DataSource::queueOperations(std::vector<op::Operation> operations)
  {
    _operationsQueue.push_back(std::move(operations));
    processQueue();
    processIntegrationQueue();
  }

  void DataSource::processQueue()
  {
    for (auto& item : _operationsQueue)
      _integrationQueue.push_back(_backend.execute(std::move(item)));

    _operationsQueue.clear();
  }

  void DataSource::processIntegrationQueue()
  {
    for (auto& item : _integrationQueue)
    {
      for (auto& result : item)
        boost::apply_visitor([this](auto& result) { return this->integrateResult(result); }, result);
    }
    _integrationQueue.clear();
  }

  void DataSource::integrateResult(op::NoResult&) { }

  void DataSource::integrateResult(op::EraseAllDataResult&)
  {
    _planning.clear();
    _hotels.clear();
  }

  void DataSource::integrateResult(op::LoadInitialDataResult& res)
  {
    _hotels = std::move(*res.hotels);
    _planning = std::move(*res.planning);
  }

  void DataSource::integrateResult(op::StoreNewReservationResult& res)
  {
    if (!_planning.canAddReservation(*res.storedReservation))
    {
      std::cerr << "Cannot add reservation " << res.storedReservation->description() << std::endl;
      return;
    }

    _planning.addReservation(std::move(res.storedReservation));
  }

  void DataSource::integrateResult(op::StoreNewHotelResult& res)
  {
    for (auto& room : res.storedHotel->rooms())
      _planning.addRoomId(room->id());
    _hotels.addHotel(std::move(res.storedHotel));
  }

  void DataSource::integrateResult(op::StoreNewPersonResult& res)
  {
    // TODO: Implement this
    std::cout << "STUB: This functionality has not yet been implemented..." << std::endl;
  }

} // namespace persistence
