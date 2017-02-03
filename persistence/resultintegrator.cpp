#include "persistence/resultintegrator.h"

namespace persistence
{
  hotel::HotelCollection& ResultIntegrator::hotels() { return _hotels; }
  const hotel::HotelCollection& ResultIntegrator::hotels() const { return _hotels; }
  hotel::PlanningBoard& ResultIntegrator::planning() { return _planning; }
  const hotel::PlanningBoard& ResultIntegrator::planning() const { return _planning; }

  void ResultIntegrator::reportResult(op::OperationResultsMessage results)
  {
    {
      std::unique_lock<std::mutex> lock(_queueMutex);
      _integrationQueue.push(std::move(results));
    }
    _resultsAvailableSignal();
  }

  boost::signals2::signal<void()>& ResultIntegrator::resultsAvailableSignal() { return _resultsAvailableSignal; }

  boost::signals2::signal<void(int)>& ResultIntegrator::resultIntegratedSignal() { return _resultIntegratedSignal; }

  void ResultIntegrator::processIntegrationQueue()
  {
    std::unique_lock<std::mutex> lock(_queueMutex);
    while (!_integrationQueue.empty())
    {
      for (auto& result : _integrationQueue.front().results)
        boost::apply_visitor([this](auto& result) { return this->integrateResult(result); }, result);
      _resultIntegratedSignal(_integrationQueue.front().uniqueId);
      _integrationQueue.pop();
    }
  }

  void ResultIntegrator::integrateResult(op::NoResult&) {}

  void ResultIntegrator::integrateResult(op::EraseAllDataResult&)
  {
    _planning.clear();
    _hotels.clear();
  }

  void ResultIntegrator::integrateResult(op::LoadInitialDataResult& res)
  {
    _hotels = std::move(*res.hotels);
    _planning = std::move(*res.planning);
  }

  void ResultIntegrator::integrateResult(op::StoreNewReservationResult& res)
  {
    if (!_planning.canAddReservation(*res.storedReservation))
    {
      std::cerr << "Cannot add reservation " << res.storedReservation->description() << std::endl;
      return;
    }

    _planning.addReservation(std::move(res.storedReservation));
  }

  void ResultIntegrator::integrateResult(op::StoreNewHotelResult& res)
  {
    for (auto& room : res.storedHotel->rooms())
      _planning.addRoomId(room->id());
    _hotels.addHotel(std::move(res.storedHotel));
  }

  void ResultIntegrator::integrateResult(op::StoreNewPersonResult& res)
  {
    // TODO: Implement this
    std::cout << "STUB: This functionality has not yet been implemented..." << std::endl;
  }

  void ResultIntegrator::integrateResult(op::DeleteReservationResult& res)
  {
    auto reservation = _planning.getReservationById(res.deletedReservationId);
    if (reservation != nullptr)
      _planning.removeReservation(reservation);
    else
      std::cerr << "Cannot remove reservation with id " << res.deletedReservationId
                << " from planning board: no such id" << std::endl;
  }

} // namespace persistence
