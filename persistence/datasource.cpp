#include "persistence/datasource.h"

#include <algorithm>
#include <iostream>

namespace persistence
{
  // TODO: Move these implementations to a backend specific file or class
  namespace {
    void executeOperation(sqlite::SqliteStorage& storage, DataSource& dataSource, op::StoreNewReservation& op)
    {
      if (op.newReservation == nullptr)
        return;
      if (!dataSource.planning().canAddReservation(*op.newReservation))
        return;

      op.newReservation->setStatus(hotel::Reservation::New);

      storage.beginTransaction();
      storage.storeNewReservationAndAtoms(*op.newReservation);
      storage.commitTransaction();

      dataSource.planning().addReservation(std::move(op.newReservation));

      std::cout << "Requesting storage of a reservation..." << std::endl;
    }

    void executeOperation(sqlite::SqliteStorage& storage, DataSource& dataSource, op::StoreNewPerson& op)
    {
      // TODO: Implement this
      std::cout << "STUB: This functionality has not yet been implemented..." << std::endl;
    }
  }


  DataSource::DataSource(const std::string& databaseFile) : _storage(databaseFile)
  {
    _hotels = _storage.loadHotels();
    _planning = _storage.loadPlanning(_hotels->allRoomIDs());
  }

  hotel::HotelCollection& DataSource::hotels() { return *_hotels; }
  const hotel::HotelCollection& DataSource::hotels() const { return *_hotels; }
  hotel::PlanningBoard& DataSource::planning() { return *_planning; }
  const hotel::PlanningBoard& DataSource::planning() const { return *_planning; }

  void DataSource::queueOperation(op::Operation operation)
  {
    boost::apply_visitor([this](auto& op) { return executeOperation(_storage, *this, op); }, operation);
  }

} // namespace persistence
