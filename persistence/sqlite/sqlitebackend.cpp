#include "persistence/sqlite/sqlitebackend.h"

#include "persistence/resultintegrator.h"

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
        auto streamPtr = uninitializedStream.get();
        _activeStreams.push_back(std::move(uninitializedStream));
        initializerFunction(*streamPtr);
      }
      _uninitializedStreams.clear();
    }

    template<class T, class Func>
    void DataStreamManager::foreachStream(Func func)
    {
      foreachStream(DataStream::GetStreamTypeFor<T>(), func);
    }

    template<class Func>
    void DataStreamManager::foreachStream(StreamableType type, Func func)
    {
      for (auto& activeStream : _activeStreams)
      {
        if (activeStream->streamType() == type)
          func(*activeStream);
      }
    }

    void DataStreamManager::addItems(ResultIntegrator &integrator, StreamableType type, const std::string &subtype, const StreamableItems items)
    {
      foreachStream(type, [&integrator, &items](DataStream& stream) {
        integrator.addStreamChange(stream.streamId(), DataStreamItemsAdded{items});
      });

    }

    void DataStreamManager::removeItems(ResultIntegrator &integrator, StreamableType type, const std::string &subtype, std::vector<int> ids)
    {
      foreachStream(type, [&integrator, &ids](DataStream& stream) {
        integrator.addStreamChange(stream.streamId(), DataStreamItemsRemoved{ids});
      });
    }

    void DataStreamManager::clear(ResultIntegrator &integrator, StreamableType type, const std::string &subtype)
    {
      foreachStream(type, [&integrator](DataStream& stream) {
        integrator.addStreamChange(stream.streamId(), DataStreamCleared{});
      });
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

    void SqliteBackend::start(ResultIntegrator& integrator)
    {
      assert(!_backendThread.joinable());
      _backendThread = std::thread([this, &integrator]() { this->threadMain(integrator); });
    }

    void SqliteBackend::stopAndJoin()
    {
      assert(_backendThread.joinable());

      _quitBackendThread = true;
      _workAvailableCondition.notify_one();
      _backendThread.join();
    }

    void SqliteBackend::threadMain(ResultIntegrator& integrator)
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
        {
          _dataStreams.initialize([this, &integrator](DataStream& stream) {
            initializeStream(stream, integrator);
            integrator.addStreamChange(stream.streamId(), DataStreamInitialized{});
          });
          _streamsUpdatedSignal();
        }

        // Process tasks
        for (auto& operationsMessage : newTasks)
        {
          op::OperationResults results;
          _storage.beginTransaction();
          for (auto& operation : operationsMessage.first)
          {
            auto result = boost::apply_visitor([this, &integrator](auto& op) { return this->executeOperation(integrator, op); }, operation);
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

    std::shared_ptr<DataStream> SqliteBackend::makeStream(StreamableType type, const std::string &service, const nlohmann::json &json)
    {
      return std::make_shared<DataStream>(type);
    }

    op::OperationResult SqliteBackend::executeOperation(ResultIntegrator& integrator, op::EraseAllData&)
    {
      _storage.deleteAll();

      _dataStreams.clear(integrator, StreamableType::Reservation, "");
      _dataStreams.clear(integrator, StreamableType::Hotel, "");

      return op::OperationResult{op::Successful, ""};
    }

    op::OperationResult SqliteBackend::executeOperation(ResultIntegrator& integrator, op::StoreNewHotel& op)
    {
      assert(op.newHotel != nullptr);
      if (op.newHotel == nullptr)
        return op::OperationResult{op::Error, "Trying to store empty hotel"};

      _storage.storeNewHotel(*op.newHotel);

      _dataStreams.addItems(integrator, StreamableType::Hotel, "", std::vector<hotel::Hotel>{{*op.newHotel}});

      return op::OperationResult{op::Successful, std::to_string(op.newHotel->id())};
    }

    op::OperationResult SqliteBackend::executeOperation(ResultIntegrator& integrator, op::StoreNewReservation& op)
    {
      if (op.newReservation == nullptr)
        return op::OperationResult{op::Error, "Trying to store empty reservation"};

      // "Unknown" is not a valid reservation status for serialization
      if (op.newReservation->status() == hotel::Reservation::Unknown)
        op.newReservation->setStatus(hotel::Reservation::New);
      _storage.storeNewReservationAndAtoms(*op.newReservation);

      _dataStreams.addItems(integrator, StreamableType::Reservation, "", std::vector<hotel::Reservation>{{*op.newReservation}});

      return op::OperationResult{op::Successful, std::to_string(op.newReservation->id())};
    }

    op::OperationResult SqliteBackend::executeOperation(ResultIntegrator& integrator, op::StoreNewPerson& op)
    {
      // TODO: Implement this
      std::cout << "STUB: This functionality has not yet been implemented..." << std::endl;

      return op::OperationResult{op::Error, "Not implemented yet!"};
    }

    op::OperationResult SqliteBackend::executeOperation(ResultIntegrator &integrator, op::DeleteReservation& op)
    {
      _storage.deleteReservationById(op.reservationId);

      _dataStreams.removeItems(integrator, StreamableType::Reservation, "", {op.reservationId});

      return op::OperationResult{op::Successful, std::to_string(op.reservationId)};
    }

    template<>
    void SqliteBackend::initializeStreamTyped<hotel::Hotel>(const DataStream& dataStream, ResultIntegrator& integrator)
    {
      auto hotels = _storage.loadHotels();

      _dataStreams.addItems(integrator, StreamableType::Hotel, "", std::move(*hotels));
    }

    template<>
    void SqliteBackend::initializeStreamTyped<hotel::Reservation>(const DataStream& dataStream, ResultIntegrator& integrator)
    {
      auto reservations = _storage.loadReservations();

      _dataStreams.addItems(integrator, StreamableType::Reservation, "", std::move(*reservations));
    }

    void SqliteBackend::initializeStream(const DataStream &dataStream, ResultIntegrator &integrator)
    {
      switch(dataStream.streamType())
      {
      case StreamableType::NullStream: return;
      case StreamableType::Hotel: return initializeStreamTyped<hotel::Hotel>(dataStream, integrator);
      case StreamableType::Reservation: return initializeStreamTyped<hotel::Reservation>(dataStream, integrator);
      }
    }

  } // namespace sqlite
} // namespace persistence
