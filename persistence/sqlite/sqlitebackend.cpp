#include "persistence/sqlite/sqlitebackend.h"

#include <cassert>

namespace persistence
{
  namespace detail
  {
    bool DataStreamManager::collectNewStreams()
    {
      assert(_uninitializedStreams.size() == 0);
      std::swap(_uninitializedStreams, _newStreams);
      return !_uninitializedStreams.empty();
    }

    template<class Func>
    void DataStreamManager::initialize(Func initializerFunction)
    {
      for (auto& uninitializedStream : _uninitializedStreams)
      {
        boost::apply_visitor([&](auto& stream) { initializerFunction(*stream); }, uninitializedStream);
        _activeStreams.push_back(std::move(uninitializedStream));
      }
      _uninitializedStreams.clear();
    }

    template<class T, class Func>
    void DataStreamManager::foreachActiveStream(Func func)
    {
      for (auto& activeStream : _activeStreams)
      {
        if (activeStream.type() == typeid(std::shared_ptr<DataStream<T>>))
          func(*boost::get<std::shared_ptr<DataStream<T>>>(activeStream));
      }
    }
  } // namespace detail

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

    void SqliteBackend::threadMain(persistence::ResultIntegrator& resultIntegrator)
    {
      while (!_quitBackendThread)
      {
        // Get the tasks we are going to process (This is the only part which is guarded by the mutex)
        std::unique_lock<std::mutex> lock(_queueMutex);
        std::vector<QueuedOperation> newTasks;
        std::swap(newTasks, _operationsQueue);
        const bool hasUninitializedStreams = _dataStreams.collectNewStreams();
        // Sleep until there is work to do
        if (newTasks.empty() && !hasUninitializedStreams)
          _workAvailableCondition.wait(lock);
        lock.unlock();

        // Initialize new data streams
        _dataStreams.initialize([this](auto& stream) { initializeStream(stream); });

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

      _dataStreams.foreachActiveStream<hotel::Reservation>([](DataStream<hotel::Reservation>& stream) { stream.clear(); });
      _dataStreams.foreachActiveStream<hotel::Hotel>([](DataStream<hotel::Hotel>& stream) { stream.clear(); });
    }

    void SqliteBackend::executeOperation(op::OperationResults& results, op::LoadInitialData&)
    {
      // TODO: Remove this! This makes no sense in a stream world!
      auto hotels = _storage.loadHotels();
      auto planning = _storage.loadPlanning(hotels->allRoomIDs());
      results.push_back(op::LoadInitialDataResult{std::move(hotels), std::move(planning)});
    }

    void SqliteBackend::executeOperation(op::OperationResults& results, op::StoreNewHotel& op)
    {
      assert(op.newHotel != nullptr);
      if (op.newHotel == nullptr)
        return;

      _storage.storeNewHotel(*op.newHotel);

      _dataStreams.foreachActiveStream<hotel::Hotel>([&op](DataStream<hotel::Hotel>& stream) { stream.addItems({*op.newHotel}); });
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

      _dataStreams.foreachActiveStream<hotel::Reservation>([&op](DataStream<hotel::Reservation>& stream) { stream.addItems({*op.newReservation}); });
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
      _dataStreams.foreachActiveStream<hotel::Reservation>([&op](DataStream<hotel::Reservation>& stream) { stream.removeItems({op.reservationId}); });
      results.push_back(op::DeleteReservationResult{op.reservationId});
    }

    void SqliteBackend::initializeStream(DataStream<hotel::Hotel>& dataStream)
    {
      // TODO: loadHotels() should already return the correct type!
      auto hotelsCollection = _storage.loadHotels();
      // Create a vector of hotels
      std::vector<hotel::Hotel> hotels;
      for (auto& hotel : hotelsCollection->hotels())
        hotels.push_back(*hotel);
      dataStream.addItems(std::move(hotels));
    }

    void SqliteBackend::initializeStream(DataStream<hotel::Reservation>& dataStream)
    {
      auto reservations = _storage.loadReservations();
      dataStream.addItems(std::move(*reservations));
    }

  } // namespace sqlite
} // namespace persistence
