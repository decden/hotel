#include "persistence/sqlite/sqlitebackend.h"

namespace persistence
{
  namespace sqlite
  {
    SqliteBackend::SqliteBackend(const std::string& databasePath) : _storage(databasePath) {}

    op::OperationResults SqliteBackend::execute(op::Operations operationBlock)
    {
      op::OperationResults results;
      _storage.beginTransaction();
      for (auto& operation : operationBlock)
        results.push_back(boost::apply_visitor([this](auto& op) { return this->executeOperation(op); }, operation));
      _storage.commitTransaction();
      return results;
    }

    op::OperationResult SqliteBackend::executeOperation(op::EraseAllData&)
    {
      _storage.deleteAll();
      return op::EraseAllDataResult();
    }

    op::OperationResult SqliteBackend::executeOperation(op::LoadInitialData&)
    {
      auto hotels = _storage.loadHotels();
      auto planning = _storage.loadPlanning(hotels->allRoomIDs());
      return op::LoadInitialDataResult { std::move(hotels), std::move(planning) };
    }

    op::OperationResult SqliteBackend::executeOperation(op::StoreNewHotel& op)
    {
      if (op.newHotel == nullptr)
        return op::NoResult();

      _storage.storeNewHotel(*op.newHotel);
      return op::StoreNewHotelResult { std::move(op.newHotel) };
    }

    op::OperationResult SqliteBackend::executeOperation(op::StoreNewReservation& op)
    {
      if (op.newReservation == nullptr)
        return op::NoResult();

      // "Unknown" is not a valid reservation status for serialization
      if (op.newReservation->status() == hotel::Reservation::Unknown)
        op.newReservation->setStatus(hotel::Reservation::New);

      _storage.storeNewReservationAndAtoms(*op.newReservation);
      return op::StoreNewReservationResult { std::move(op.newReservation) };
    }

    op::OperationResult SqliteBackend::executeOperation(op::StoreNewPerson& op)
    {
      // TODO: Implement this
      std::cout << "STUB: This functionality has not yet been implemented..." << std::endl;

      return op::NoResult();
    }

    op::OperationResult SqliteBackend::executeOperation(op::DeleteReservation& op)
    {
      _storage.deleteReservationById(op.reservationId);
      return op::DeleteReservationResult { op.reservationId };
    }

  } // namespace sqlite
} // namespace persistence
