#include "persistence/datasource.h"

#include <algorithm>
#include <iostream>

namespace persistence
{
  // TODO: Move these implementations to a backend specific file or class
  namespace {
    op::OperationResult executeOperation(sqlite::SqliteStorage& storage, DataSource& dataSource, op::EraseAllData&)
    {
      storage.deleteAll();
      return op::EraseAllDataResult();
    }

    op::OperationResult executeOperation(sqlite::SqliteStorage& storage, DataSource& dataSource, op::LoadInitialData&)
    {
      auto hotels = storage.loadHotels();
      auto planning = storage.loadPlanning(hotels->allRoomIDs());
      return op::LoadInitialDataResult { std::move(hotels), std::move(planning) };
    }

    op::OperationResult executeOperation(sqlite::SqliteStorage& storage, DataSource& dataSource, op::StoreNewHotel& op)
    {
      if (op.newHotel == nullptr)
        return op::NoResult();

      storage.storeNewHotel(*op.newHotel);
      return op::StoreNewHotelResult { std::move(op.newHotel) };
    }

    op::OperationResult executeOperation(sqlite::SqliteStorage& storage, DataSource& dataSource, op::StoreNewReservation& op)
    {
      if (op.newReservation == nullptr)
        return op::NoResult();
      if (!dataSource.planning().canAddReservation(*op.newReservation))
        return op::NoResult();

      // "Unknown" is not a valid reservation status for serialization
      if (op.newReservation->status() == hotel::Reservation::Unknown)
        op.newReservation->setStatus(hotel::Reservation::New);

      storage.storeNewReservationAndAtoms(*op.newReservation);
      return op::StoreNewReservationResult { std::move(op.newReservation) };
    }

    op::OperationResult executeOperation(sqlite::SqliteStorage& storage, DataSource& dataSource, op::StoreNewPerson& op)
    {
      // TODO: Implement this
      std::cout << "STUB: This functionality has not yet been implemented..." << std::endl;

      return op::NoResult();
    }

    void integrateResult(DataSource&, op::NoResult&) {}

    void integrateResult(DataSource& dataSource, op::EraseAllDataResult&)
    {
      dataSource.hotels().clear();
      dataSource.planning().clear();
    }

    void integrateResult(DataSource& dataSource, op::LoadInitialDataResult& res)
    {
      dataSource.hotels() = std::move(*res.hotels);
      dataSource.planning() = std::move(*res.planning);
    }

    void integrateResult(DataSource& dataSource, op::StoreNewReservationResult& res)
    {
      dataSource.planning().addReservation(std::move(res.storedReservation));
    }

    void integrateResult(DataSource& dataSource, op::StoreNewHotelResult& res)
    {
      for (auto& room : res.storedHotel->rooms())
        dataSource.planning().addRoomId(room->id());
      dataSource.hotels().addHotel(std::move(res.storedHotel));
    }

    void integrateResult(DataSource& dataSource, op::StoreNewPersonResult& res)
    {
      // TODO: Implement this
      std::cout << "STUB: This functionality has not yet been implemented..." << std::endl;
    }
  }


  DataSource::DataSource(const std::string& databaseFile) : _storage(databaseFile)
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
    {
      op::OperationResults results;
      _storage.beginTransaction();
      for (auto& operation : item)
        results.push_back(boost::apply_visitor([this](auto& op) { return executeOperation(_storage, *this, op); }, operation));
      _storage.commitTransaction();

      _integrationQueue.push_back(std::move(results));
    }
    _operationsQueue.clear();
  }

  void DataSource::processIntegrationQueue()
  {
    for (auto& item : _integrationQueue)
    {
      for (auto& result : item)
        boost::apply_visitor([this](auto& result) { return integrateResult(*this, result); }, result);
    }
    _integrationQueue.clear();
  }

} // namespace persistence
