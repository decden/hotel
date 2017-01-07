#include "persistence/datasource.h"

#include <algorithm>
#include <iostream>

namespace persistence
{
  // TODO: Move these implementations to a backend specific file or class
  namespace {
    void executeOperation(sqlite::SqliteStorage& storage, DataSource& dataSource, op::EraseAllData&)
    {
      storage.deleteAll();
      dataSource.planning().clear();
      dataSource.hotels().clear();
    }

    void executeOperation(sqlite::SqliteStorage& storage, DataSource& dataSource, op::LoadInitialData&)
    {
      auto hotels = storage.loadHotels();
      auto planning = storage.loadPlanning(hotels->allRoomIDs());

      dataSource.hotels() = std::move(*hotels);
      dataSource.planning() = std::move(*planning);
    }

    void executeOperation(sqlite::SqliteStorage& storage, DataSource& dataSource, op::StoreNewHotel& op)
    {
      if (op.newHotel == nullptr)
        return;

      storage.storeNewHotel(*op.newHotel);
      for (auto& room : op.newHotel->rooms())
        dataSource.planning().addRoomId(room->id());
      dataSource.hotels().addHotel(std::move(op.newHotel));
    }

    void executeOperation(sqlite::SqliteStorage& storage, DataSource& dataSource, op::StoreNewReservation& op)
    {
      if (op.newReservation == nullptr)
        return;
      if (!dataSource.planning().canAddReservation(*op.newReservation))
        return;

      if (op.newReservation->status() == hotel::Reservation::Unknown)
        op.newReservation->setStatus(hotel::Reservation::New);

      storage.storeNewReservationAndAtoms(*op.newReservation);
      dataSource.planning().addReservation(std::move(op.newReservation));
    }

    void executeOperation(sqlite::SqliteStorage& storage, DataSource& dataSource, op::StoreNewPerson& op)
    {
      // TODO: Implement this
      std::cout << "STUB: This functionality has not yet been implemented..." << std::endl;
    }
  }


  DataSource::DataSource(const std::string& databaseFile) : _storage(databaseFile)
  {
    queueOperation(op::LoadInitialData());
    processQueue();
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
  }

  void DataSource::queueOperations(std::vector<op::Operation> operations)
  {
    _operationsQueue.push_back(std::move(operations));
    processQueue();
  }

  void DataSource::processQueue()
  {
    for (auto& item : _operationsQueue)
    {
      _storage.beginTransaction();
      for (auto& operation : item)
        boost::apply_visitor([this](auto& op) { return executeOperation(_storage, *this, op); }, operation);
      _storage.commitTransaction();
    }
    _operationsQueue.clear();
  }

} // namespace persistence
