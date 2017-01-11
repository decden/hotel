#include "persistence/sqlite/sqlitebackend.h"

#include "persistence/datasource.h"

#include <cassert>

namespace persistence
{
  namespace sqlite
  {
    SqliteBackend::SqliteBackend(const std::string& databasePath)
        : _storage(databasePath), _backendThread(), _quitBackendThread(false), _workAvailableCondition(), _queueMutex(),
          _operationsQueue()
    {
    }

    void SqliteBackend::queueOperation(op::Operations operationBlock)
    {
      std::unique_lock<std::mutex> lock(_queueMutex);
      _operationsQueue.push(std::move(operationBlock));

      lock.unlock();
      _workAvailableCondition.notify_one();

    }

    void SqliteBackend::start(persistence::DataSource &dataSource)
    {
      assert(!_backendThread.joinable());
      _backendThread = std::thread([this, &dataSource]() { this->threadMain(dataSource); });
    }

    void SqliteBackend::stopAndJoin()
    {
      assert(_backendThread.joinable());

      _quitBackendThread = true;
      _workAvailableCondition.notify_one();
      _backendThread.join();
    }

    void SqliteBackend::threadMain(persistence::DataSource &dataSource)
    {
      while (!_quitBackendThread)
      {
        std::unique_lock<std::mutex> lock(_queueMutex);
        if (_operationsQueue.empty())
        {
          _workAvailableCondition.wait(lock);
        }
        else
        {
          auto operationBlock = std::move(_operationsQueue.front());
          _operationsQueue.pop();
          lock.unlock();

          op::OperationResults results;
          _storage.beginTransaction();
          for (auto& operation : operationBlock)
            results.push_back(boost::apply_visitor([this](auto& op) { return this->executeOperation(op); }, operation));
          _storage.commitTransaction();
          dataSource.reportResult(std::move(results));
        }
      }
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
