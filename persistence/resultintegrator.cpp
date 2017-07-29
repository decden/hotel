#include "persistence/resultintegrator.h"

#include <algorithm>

namespace persistence
{
  void ResultIntegrator::processIntegrationQueue()
  {
    std::unique_lock<std::mutex> lock(_queueMutex);

    // First, integrate all of the changes in the data streams...
    {
      // Remove invalid streams (streams which no longer have a listener)
      _dataStreams.erase(std::remove_if(_dataStreams.begin(), _dataStreams.end(),
                                        [](auto& stream) { return boost::apply_visitor([](auto& stream) { return !stream->isValid(); }, stream); }),
                              _dataStreams.end());

      for (auto& stream : _dataStreams)
        boost::apply_visitor([](auto& stream) { stream->integrateChanges(); }, stream);
    }

    // Second, integrate all of the results for tasks...
    {
      // Move all of the completed tasks to the end of the list
      auto readyBegin = std::stable_partition(begin(_integrationQueue), end(_integrationQueue),
                                              [](auto& task) { return !task.completed(); });

      // For each of the completed tasks: integrate it!
      for (auto it = readyBegin; it != end(_integrationQueue); ++it)
      {
        for (auto& result : it->results())
          boost::apply_visitor([this](auto& result) { return this->integrateResult(result); }, result);
      }

      // Erase all completed tasks
      _integrationQueue.erase(readyBegin, end(_integrationQueue));
    }
  }

  void ResultIntegrator::addPendingOperation(op::Task<op::OperationResults> task)
  {
    std::unique_lock<std::mutex> lock(_queueMutex);
    _integrationQueue.push_back(std::move(task));
  }

  size_t ResultIntegrator::pendingOperationsCount() const
  {
    return _integrationQueue.size();
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
