#include "persistence/sqlite/sqlitebackend.h"

#include <cassert>

namespace persistence
{
  namespace sqlite
  {
    SqliteBackend::SqliteBackend(const std::string& databasePath)
        : _storage(databasePath), _nextOperationId(1), _backendThread(), _quitBackendThread(false),
          _workAvailableCondition(), _queueMutex(), _operationsQueue()
    {
    }

    op::Task<op::OperationResults> SqliteBackend::queueOperation(op::Operations operations)
    {
      // Create a task
      auto sharedState = std::make_shared<op::TaskSharedState<op::OperationResults>>(_nextOperationId++);
      op::Task<op::OperationResults> task(sharedState);

      std::unique_lock<std::mutex> lock(_queueMutex);
      auto pair = QueuedOperation{std::move(operations), sharedState};
      _operationsQueue.push_back(std::move(pair));
      lock.unlock();
      _workAvailableCondition.notify_one();

      return task;
    }

    void SqliteBackend::start(persistence::ResultIntegrator& resultIntegrator)
    {
      assert(!_backendThread.joinable());
      _backendThread = std::thread([this, &resultIntegrator]() { this->threadMain(resultIntegrator); });
    }

    void SqliteBackend::stopAndJoin()
    {
      assert(_backendThread.joinable());

      _quitBackendThread = true;
      _workAvailableCondition.notify_one();
      _backendThread.join();
    }

    std::shared_ptr<DataStream<hotel::Hotel>> SqliteBackend::createStream(DataStreamObserver<hotel::Hotel> *observer)
    {
      std::unique_lock<std::mutex> lock(_queueMutex);
      auto sharedState = std::make_shared<DataStream<hotel::Hotel>>(_nextStreamId++, observer);
      _newHotelStreams.push_back(sharedState);
      lock.unlock();

      _workAvailableCondition.notify_one();

      return sharedState;
    }

    void SqliteBackend::threadMain(persistence::ResultIntegrator& resultIntegrator)
    {
      while (!_quitBackendThread)
      {
        // Get the tasks we are going to process (This is the only part which is guarded by the mutex)
        std::unique_lock<std::mutex> lock(_queueMutex);
        std::vector<QueuedOperation> newTasks;
        std::vector<std::shared_ptr<DataStream<hotel::Hotel>>> newStreams;
        std::swap(newTasks, _operationsQueue);
        std::swap(newStreams, _newHotelStreams);
        // Sleep until there is work to do
        if (newStreams.empty() && newTasks.empty())
          _workAvailableCondition.wait(lock);
        lock.unlock();

        // Initialize new data streams
        if (!newStreams.empty())
        {
          for (auto& stream : newStreams)
            initializeStream(*stream);

          std::copy(newStreams.begin(), newStreams.end(), std::back_inserter(_activeHotelStreams));
        }

        // Process tasks
        for (auto& operationsMessage : newTasks)
        {
          op::OperationResults results;
          _storage.beginTransaction();
          for (auto& operation : operationsMessage.first)
            boost::apply_visitor([this, &results](auto& op) { return this->executeOperation(results, op); }, operation);
          _storage.commitTransaction();
          operationsMessage.second->setCompleted(std::move(results));
          _taskCompletedSignal(operationsMessage.second->uniqueId());
        }

        // Notify the client that the streams may have new data...
        // TODO: Signaling here is not ideal. We might trigger the signal even if nothing changed with the stream.
        _streamsUpdatedSignal();
      }
    }

    void SqliteBackend::executeOperation(op::OperationResults& results, op::EraseAllData&)
    {
      _storage.deleteAll();
      results.push_back(op::EraseAllDataResult());
    }

    void SqliteBackend::executeOperation(op::OperationResults& results, op::LoadInitialData&)
    {
      auto hotels = _storage.loadHotels();
      auto planning = _storage.loadPlanning(hotels->allRoomIDs());
      results.push_back(op::LoadInitialDataResult{std::move(hotels), std::move(planning)});
    }

    void SqliteBackend::executeOperation(op::OperationResults& results, op::StoreNewHotel& op)
    {
      assert(op.newHotel != nullptr);
      if (op.newHotel == nullptr)
        return;

      for (auto& stream : _activeHotelStreams)
        stream->addItems({*op.newHotel});

      _storage.storeNewHotel(*op.newHotel);
      results.push_back(op::StoreNewHotelResult{std::move(op.newHotel)});
    }

    void SqliteBackend::executeOperation(op::OperationResults& results, op::StoreNewReservation& op)
    {
      if (op.newReservation == nullptr)
        results.push_back(op::NoResult());

      // "Unknown" is not a valid reservation status for serialization
      if (op.newReservation->status() == hotel::Reservation::Unknown)
        op.newReservation->setStatus(hotel::Reservation::New);

      _storage.storeNewReservationAndAtoms(*op.newReservation);
      results.push_back(op::StoreNewReservationResult{std::move(op.newReservation)});
    }

    void SqliteBackend::executeOperation(op::OperationResults& results, op::StoreNewPerson& op)
    {
      // TODO: Implement this
      std::cout << "STUB: This functionality has not yet been implemented..." << std::endl;

      results.push_back(op::NoResult());
    }

    void SqliteBackend::executeOperation(op::OperationResults& results, op::DeleteReservation& op)
    {
      _storage.deleteReservationById(op.reservationId);
      results.push_back(op::DeleteReservationResult{op.reservationId});
    }

    void SqliteBackend::initializeStream(DataStream<hotel::Hotel> &dataStream)
    {
      // TODO: loadHotels() should already return the correct type!
      auto hotelsCollection = _storage.loadHotels();
      // Create a vector of hotels
      std::vector<hotel::Hotel> hotels;
      for (auto& hotel : hotelsCollection->hotels())
        hotels.push_back(*hotel);
      std::cout << "Initialized stream with " << hotels.size() << " items" << std::endl;
      dataStream.addItems(std::move(hotels));
    }

  } // namespace sqlite
} // namespace persistence
