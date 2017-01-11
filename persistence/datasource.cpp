#include "persistence/datasource.h"

#include <algorithm>
#include <iostream>

namespace persistence
{
  DataSource::DataSource(const std::string& databaseFile) : _backend(databaseFile)
  {
    _backend.start(*this);
    queueOperation(op::LoadInitialData());
  }

  DataSource::~DataSource()
  {
    _backend.stopAndJoin();
  }

  hotel::HotelCollection& DataSource::hotels() { return _hotels; }
  const hotel::HotelCollection& DataSource::hotels() const { return _hotels; }
  hotel::PlanningBoard& DataSource::planning() { return _planning; }
  const hotel::PlanningBoard& DataSource::planning() const { return _planning; }

  void DataSource::queueOperation(op::Operation operation)
  {
    op::Operations item;
    item.push_back(std::move(operation));
    _backend.queueOperation(std::move(item));
  }

  void DataSource::queueOperations(op::Operations operations)
  {
    _backend.queueOperation(std::move(operations));
  }

  void DataSource::reportResult(op::OperationResults results)
  {
    std::unique_lock<std::mutex> lock(_queueMutex);
    _integrationQueue.push(std::move(results));
  }

  void DataSource::processIntegrationQueue()
  {
    std::unique_lock<std::mutex> lock(_queueMutex);
    while (!_integrationQueue.empty())
    {
      for (auto& result : _integrationQueue.front())
        boost::apply_visitor([this](auto& result) { return this->integrateResult(result); }, result);
      _integrationQueue.pop();
    }
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

  void DataSource::integrateResult(op::DeleteReservationResult &res)
  {
    auto reservation = _planning.getReservationById(res.deletedReservationId);
    if (reservation != nullptr)
      _planning.removeReservation(reservation);
    else
      std::cerr << "Cannot remove reservation with id " << res.deletedReservationId << " from planning board: no such id" << std::endl;

  }

} // namespace persistence
