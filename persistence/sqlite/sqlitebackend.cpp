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
        initializerFunction(*uninitializedStream);
        _activeStreams.push_back(std::move(uninitializedStream));
      }
      _uninitializedStreams.clear();
    }

    template<class T, class Func>
    void DataStreamManager::foreachActiveStream(Func func)
    {
      for (auto& activeStream : _activeStreams)
      {
        if (activeStream->streamType() == DataStream::GetStreamTypeFor<T>())
          func(*activeStream);
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
        _dataStreams.initialize([this](DataStream& stream) {
          initializeStream(stream);
          stream.setInitialized();
        });

        // Process tasks
        for (auto& operationsMessage : newTasks)
        {
          op::OperationResults results;
          _storage.beginTransaction();
          for (auto& operation : operationsMessage.first)
          {
            auto result = boost::apply_visitor([this](auto& op) { return this->executeOperation(op); }, operation);
            results.push_back(result);
          }
          _storage.commitTransaction();
          operationsMessage.second->setCompleted(std::move(results));
          _taskCompletedSignal(operationsMessage.second->uniqueId());
        }

        // Notify the client that the streams may have new data...
        // TODO: Signaling here is not ideal. We might trigger the signal even if nothing changed with the stream.
        _streamsUpdatedSignal();
      }
    }

    op::OperationResult SqliteBackend::executeOperation(op::EraseAllData&)
    {
      _storage.deleteAll();
      _dataStreams.foreachActiveStream<hotel::Reservation>([](DataStream& stream) { stream.clear(); });
      _dataStreams.foreachActiveStream<hotel::Hotel>([](DataStream& stream) { stream.clear(); });
      return op::OperationResult{op::Successful, ""};
    }

    op::OperationResult SqliteBackend::executeOperation(op::StoreNewHotel& op)
    {
      assert(op.newHotel != nullptr);
      if (op.newHotel == nullptr)
        return op::OperationResult{op::Error, "Trying to store empty hotel"};

      _storage.storeNewHotel(*op.newHotel);

      _dataStreams.foreachActiveStream<hotel::Hotel>([&op](DataStream& stream) { stream.addItems(std::vector<hotel::Hotel>{*op.newHotel}); });
      return op::OperationResult{op::Successful, std::to_string(op.newHotel->id())};
    }

    op::OperationResult SqliteBackend::executeOperation(op::StoreNewReservation& op)
    {
      if (op.newReservation == nullptr)
        return op::OperationResult{op::Error, "Trying to store empty reservation"};

      // "Unknown" is not a valid reservation status for serialization
      if (op.newReservation->status() == hotel::Reservation::Unknown)
        op.newReservation->setStatus(hotel::Reservation::New);
      _storage.storeNewReservationAndAtoms(*op.newReservation);

      _dataStreams.foreachActiveStream<hotel::Reservation>([&op](DataStream& stream) { stream.addItems(std::vector<hotel::Reservation>{*op.newReservation}); });
      return op::OperationResult{op::Successful, std::to_string(op.newReservation->id())};
    }

    op::OperationResult SqliteBackend::executeOperation(op::StoreNewPerson& op)
    {
      // TODO: Implement this
      std::cout << "STUB: This functionality has not yet been implemented..." << std::endl;

      return op::OperationResult{op::Error, "Not implemented yet!"};
    }

    op::OperationResult SqliteBackend::executeOperation(op::DeleteReservation& op)
    {
      _storage.deleteReservationById(op.reservationId);
      _dataStreams.foreachActiveStream<hotel::Reservation>([&op](DataStream& stream) { stream.removeItems({op.reservationId}); });

      return op::OperationResult{op::Successful, std::to_string(op.reservationId)};
    }

    template<>
    void SqliteBackend::initializeStreamTyped<hotel::Hotel>(DataStream& dataStream)
    {
      auto hotels = _storage.loadHotels();
      dataStream.addItems(std::move(*hotels));
    }

    template<>
    void SqliteBackend::initializeStreamTyped<hotel::Reservation>(DataStream& dataStream)
    {
      auto reservations = _storage.loadReservations();
      dataStream.addItems(std::move(*reservations));
    }

    void SqliteBackend::initializeStream(DataStream &dataStream)
    {
      switch(dataStream.streamType())
      {
      case StreamableType::Hotel: return initializeStreamTyped<hotel::Hotel>(dataStream);
      case StreamableType::Reservation: return initializeStreamTyped<hotel::Reservation>(dataStream);
      }
    }

  } // namespace sqlite
} // namespace persistence
