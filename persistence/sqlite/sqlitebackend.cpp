#include "persistence/sqlite/sqlitebackend.h"

#include "persistence/changequeue.h"

#include <cassert>

namespace persistence
{
  namespace detail
  {
    class DefaultDataStreamHandler : public DataStreamHandler
    {
    public:
      virtual ~DefaultDataStreamHandler() {}
      virtual void initialize(DataStream& stream, ChangeQueue& changeQueue, sqlite::SqliteStorage& storage) override
      {
        switch(stream.streamType())
        {
        case StreamableType::NullStream: return;
        case StreamableType::Hotel: return initializeTyped<hotel::Hotel>(stream, changeQueue, storage);
        case StreamableType::Reservation: return initializeTyped<hotel::Reservation>(stream, changeQueue, storage);
        }
      }

      virtual void addItems(DataStream& stream, ChangeQueue& changeQueue, const StreamableItems& items) override
      {
        changeQueue.addStreamChange(stream.streamId(), DataStreamItemsAdded{items});
      }

      virtual void removeItems(DataStream& stream, ChangeQueue& changeQueue, const std::vector<int> ids) override
      {
        changeQueue.addStreamChange(stream.streamId(), DataStreamItemsRemoved{ids});
      }

      virtual void clear(DataStream& stream, ChangeQueue& changeQueue) override
      {
        changeQueue.addStreamChange(stream.streamId(), DataStreamCleared{});
      }

    private:
      template <class T>
      void initializeTyped(DataStream& stream, ChangeQueue& changeQueue, sqlite::SqliteStorage& storage)
      {
        auto items = storage.loadAll<T>();
        changeQueue.addStreamChange(stream.streamId(), DataStreamItemsAdded{std::move(items)});
      }
    };


    class SingleIdDataStreamHandler : public DataStreamHandler
    {
    public:
      virtual ~SingleIdDataStreamHandler() {}
      virtual void initialize(DataStream& stream, ChangeQueue& changeQueue, sqlite::SqliteStorage& storage) override
      {
        switch(stream.streamType())
        {
        case StreamableType::NullStream: return;
        case StreamableType::Hotel: return initializeTyped<hotel::Hotel>(stream, changeQueue, storage);
        case StreamableType::Reservation: return initializeTyped<hotel::Reservation>(stream, changeQueue, storage);
        }
      }

      virtual void addItems(DataStream& stream, ChangeQueue& changeQueue, const StreamableItems& items) override
      {
        auto id = stream.streamOptions()["id"];
        auto filteredItems = filter(items, id);
        bool isEmpty = boost::apply_visitor([](const auto& items) { return items.empty(); }, items);
        if (isEmpty)
          changeQueue.addStreamChange(stream.streamId(), DataStreamItemsAdded{filteredItems});
      }

      virtual void removeItems(DataStream& stream, ChangeQueue& changeQueue, const std::vector<int> ids) override
      {
        int id = stream.streamOptions()["id"];
        if (std::any_of(ids.begin(), ids.end(), [id](int item) { return item == id; }))
          changeQueue.addStreamChange(stream.streamId(), DataStreamItemsRemoved{{id}});
      }

      virtual void clear(DataStream& stream, ChangeQueue& changeQueue) override
      {
        changeQueue.addStreamChange(stream.streamId(), DataStreamCleared{});
      }

    private:
      StreamableItems filter(const StreamableItems& items, int id)
      {
        return boost::apply_visitor([id](const auto& items) -> StreamableItems {
          typename std::remove_const<typename std::remove_reference<decltype(items)>::type>::type filteredItems;
          std::copy_if(items.begin(), items.end(), std::back_inserter(filteredItems), [id](const auto& item) { return item.id() == id; });
          return filteredItems;
        }, items);
      }

      template <class T>
      void initializeTyped(DataStream& stream, ChangeQueue& changeQueue, sqlite::SqliteStorage& storage)
      {
        auto id = stream.streamOptions()["id"];
        auto item = storage.loadById<T>(id);
        if (item != boost::none)
        {
          std::vector<T> items({*item});
          changeQueue.addStreamChange(stream.streamId(), DataStreamItemsAdded{std::move(items)});
        }
      }
    };


    DataStreamManager::DataStreamManager()
    {
      _streamHandlers[HandlerKey{StreamableType::NullStream, ""}] = std::make_unique<DefaultDataStreamHandler>();
      _streamHandlers[HandlerKey{StreamableType::Hotel, ""}] = std::make_unique<DefaultDataStreamHandler>();
      _streamHandlers[HandlerKey{StreamableType::Hotel, "hotel.by_id"}] = std::make_unique<SingleIdDataStreamHandler>();
      _streamHandlers[HandlerKey{StreamableType::Reservation, ""}] = std::make_unique<DefaultDataStreamHandler>();
      _streamHandlers[HandlerKey{StreamableType::Reservation, "reservation.by_id"}] = std::make_unique<SingleIdDataStreamHandler>();
    }

    bool DataStreamManager::collectNewStreams()
    {
      assert(_uninitializedStreams.size() == 0);
      std::swap(_uninitializedStreams, _newStreams);
      return !_uninitializedStreams.empty();
    }

    void DataStreamManager::initialize(ChangeQueue &changeQueue, sqlite::SqliteStorage &storage)
    {
      for (auto& uninitializedStream : _uninitializedStreams)
      {
        auto streamPtr = uninitializedStream.get();
        auto streamHandler = findHandler(*streamPtr);
        _activeStreams.push_back(std::move(uninitializedStream));
        if (streamHandler)
          streamHandler->initialize(*streamPtr, changeQueue, storage);
        else
          std::cerr << "Cannot initialize stream, because there is no handler registered" << std::endl;
        changeQueue.addStreamChange(streamPtr->streamId(), DataStreamInitialized{});
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
        auto handler = findHandler(*activeStream);
        assert(handler != nullptr);
        if (activeStream->streamType() == type && handler != nullptr)
          func(*activeStream, *handler);
      }
    }

    void DataStreamManager::addItems(ChangeQueue &changeQueue, StreamableType type, const StreamableItems items)
    {
      foreachStream(type, [&changeQueue, &items](DataStream& stream, DataStreamHandler& handler) {
        handler.addItems(stream, changeQueue, items);
      });
    }

    void DataStreamManager::removeItems(ChangeQueue &changeQueue, StreamableType type, std::vector<int> ids)
    {
      foreachStream(type, [&changeQueue, &ids](DataStream& stream, DataStreamHandler& handler) {
        handler.removeItems(stream, changeQueue, ids);
      });
    }

    void DataStreamManager::clear(ChangeQueue &changeQueue, StreamableType type)
    {
      foreachStream(type, [&changeQueue](DataStream& stream, DataStreamHandler& handler) {
        handler.clear(stream, changeQueue);
      });
    }

    DataStreamHandler *DataStreamManager::findHandler(const DataStream &stream)
    {
      auto it = _streamHandlers.find({stream.streamType(), stream.streamEndpoint()});
      return (it != _streamHandlers.end()) ? it->second.get() : nullptr;
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
      _changeQueue.addTask(sharedState);

      std::unique_lock<std::mutex> lock(_queueMutex);
      auto pair = QueuedOperation{std::move(operations), sharedState};
      _operationsQueue.push_back(std::move(pair));
      lock.unlock();
      _workAvailableCondition.notify_one();

      return task;
    }

    void SqliteBackend::start()
    {
      assert(!_backendThread.joinable());
      _backendThread = std::thread([this]() { this->threadMain(); });
    }

    void SqliteBackend::stopAndJoin()
    {
      assert(_backendThread.joinable());

      std::unique_lock<std::mutex> lock(_queueMutex);
      _quitBackendThread = true;
      _workAvailableCondition.notify_all();
      lock.unlock();

      _backendThread.join();
    }

    void SqliteBackend::threadMain()
    {
      while (!_quitBackendThread)
      {
        // Get the tasks we are going to process (This is the only part which is guarded by the mutex)
        std::unique_lock<std::mutex> lock(_queueMutex);
        std::vector<QueuedOperation> newTasks;
        std::swap(newTasks, _operationsQueue);
        const bool hasUninitializedStreams = _dataStreams.collectNewStreams();
        // Sleep until there is work to do
        if (!_quitBackendThread && newTasks.empty() && !hasUninitializedStreams)
          _workAvailableCondition.wait(lock);
        lock.unlock();

        // Initialize new data streams
        _dataStreams.initialize(_changeQueue, _storage);

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
          _changeQueue.taskCompleted(operationsMessage.second->uniqueId());
        }
      }
    }

    std::shared_ptr<DataStream> SqliteBackend::makeStream(StreamableType type, const std::string& service,
                                                          const nlohmann::json& options)
    {
      return std::make_shared<DataStream>(type, service, options);
    }

    op::OperationResult SqliteBackend::executeOperation(op::EraseAllData&)
    {
      _storage.deleteAll();

      _dataStreams.clear(_changeQueue, StreamableType::Reservation);
      _dataStreams.clear(_changeQueue, StreamableType::Hotel);

      return op::OperationResult{op::Successful, ""};
    }

    op::OperationResult SqliteBackend::executeOperation(op::StoreNewHotel& op)
    {
      assert(op.newHotel != nullptr);
      if (op.newHotel == nullptr)
        return op::OperationResult{op::Error, "Trying to store empty hotel"};

      _storage.storeNewHotel(*op.newHotel);

      _dataStreams.addItems(_changeQueue, StreamableType::Hotel, std::vector<hotel::Hotel>{{*op.newHotel}});

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

      _dataStreams.addItems(_changeQueue, StreamableType::Reservation, std::vector<hotel::Reservation>{{*op.newReservation}});

      return op::OperationResult{op::Successful, std::to_string(op.newReservation->id())};
    }

    op::OperationResult SqliteBackend::executeOperation(op::StoreNewPerson &op)
    {
      // TODO: Implement this
      std::cout << "STUB: This functionality has not yet been implemented..." << std::endl;

      return op::OperationResult{op::Error, "Not implemented yet!"};
    }

    op::OperationResult SqliteBackend::executeOperation(op::UpdateReservation &op)
    {
      if (op.updatedReservation == nullptr)
        return op::OperationResult{op::Error, "Trying to update empty reservation"};
      if (op.updatedReservation->id() == 0)
        return op::OperationResult{op::Error, "Cannot update reservation without id"};

      // "Unknown" is not a valid reservation status for serialization
      if (op.updatedReservation->status() == hotel::Reservation::Unknown)
        op.updatedReservation->setStatus(hotel::Reservation::New);
      if (!_storage.update<hotel::Reservation>(*op.updatedReservation))
        return op::OperationResult{op::Error, "Could not update reservation (internal db error)"};

      // TODO: We should have a proper update event here instead of removing the reservation and readding it...
      _dataStreams.removeItems(_changeQueue, StreamableType::Reservation, std::vector<int>({op.updatedReservation->id()}));
      _dataStreams.addItems(_changeQueue, StreamableType::Reservation, std::vector<hotel::Reservation>{{*op.updatedReservation}});

      return op::OperationResult{op::Successful, ""};
    }

    op::OperationResult SqliteBackend::executeOperation(op::DeleteReservation& op)
    {
      _storage.deleteReservationById(op.reservationId);

      _dataStreams.removeItems(_changeQueue, StreamableType::Reservation, {op.reservationId});

      return op::OperationResult{op::Successful, std::to_string(op.reservationId)};
    }

  } // namespace sqlite
} // namespace persistence
